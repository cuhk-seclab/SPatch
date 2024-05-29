static ssize_t ocfs2_file_write_iter(struct kiocb *iocb,
				    struct iov_iter *from)
{
	int direct_io, appending, rw_level, have_alloc_sem  = 0;
	int can_do_direct, has_refcount = 0;
	ssize_t written = 0;
	ssize_t ret;
	size_t count = iov_iter_count(from), orig_count;
	loff_t old_size;
	u32 old_clusters;
	struct file *file = iocb->ki_filp;
	struct inode *inode = file_inode(file);
	struct ocfs2_super *osb = OCFS2_SB(inode->i_sb);
	int full_coherency = !(osb->s_mount_opt &
			       OCFS2_MOUNT_COHERENCY_BUFFERED);
	int unaligned_dio = 0;
	int dropped_dio = 0;

	trace_ocfs2_file_aio_write(inode, file, file->f_path.dentry,
		(unsigned long long)OCFS2_I(inode)->ip_blkno,
		file->f_path.dentry->d_name.len,
		file->f_path.dentry->d_name.name,
		(unsigned int)from->nr_segs);	/* GRRRRR */

	if (count == 0)
		return 0;

	appending = iocb->ki_flags & IOCB_APPEND ? 1 : 0;
	direct_io = iocb->ki_flags & IOCB_DIRECT ? 1 : 0;

	mutex_lock(&inode->i_mutex);

	ocfs2_iocb_clear_sem_locked(iocb);

relock:
	/* to match setattr's i_mutex -> rw_lock ordering */
	if (direct_io) {
		have_alloc_sem = 1;
		/* communicate with ocfs2_dio_end_io */
		ocfs2_iocb_set_sem_locked(iocb);
	}

	/*
	 * Concurrent O_DIRECT writes are allowed with
	 * mount_option "coherency=buffered".
	 */
	rw_level = (!direct_io || full_coherency);

	ret = ocfs2_rw_lock(inode, rw_level);
	if (ret < 0) {
		mlog_errno(ret);
		goto out_sems;
	}

	/*
	 * O_DIRECT writes with "coherency=full" need to take EX cluster
	 * inode_lock to guarantee coherency.
	 */
	if (direct_io && full_coherency) {
		/*
		 * We need to take and drop the inode lock to force
		 * other nodes to drop their caches.  Buffered I/O
		 * already does this in write_begin().
		 */
		ret = ocfs2_inode_lock(inode, NULL, 1);
		if (ret < 0) {
			mlog_errno(ret);
			goto out;
		}

		ocfs2_inode_unlock(inode, 1);
	}

	orig_count = iov_iter_count(from);
	ret = generic_write_checks(iocb, from);
	if (ret <= 0) {
		if (ret)
			mlog_errno(ret);
		goto out;
	}
	count = ret;

	can_do_direct = direct_io;
	ret = ocfs2_prepare_inode_for_write(file, iocb->ki_pos, count, appending,
					    &can_do_direct, &has_refcount);
	if (ret < 0) {
		mlog_errno(ret);
		goto out;
	}

	if (direct_io && !is_sync_kiocb(iocb))
		unaligned_dio = ocfs2_is_io_unaligned(inode, count, iocb->ki_pos);

	/*
	 * We can't complete the direct I/O as requested, fall back to
	 * buffered I/O.
	 */
	if (direct_io && !can_do_direct) {
		ocfs2_rw_unlock(inode, rw_level);

		have_alloc_sem = 0;
		rw_level = -1;

		direct_io = 0;
		iocb->ki_flags &= ~IOCB_DIRECT;
		iov_iter_reexpand(from, orig_count);
		dropped_dio = 1;
		goto relock;
	}

	if (unaligned_dio) {
		/*
		 * Wait on previous unaligned aio to complete before
		 * proceeding.
		 */
		mutex_lock(&OCFS2_I(inode)->ip_unaligned_aio);
		/* Mark the iocb as needing an unlock in ocfs2_dio_end_io */
		ocfs2_iocb_set_unaligned_aio(iocb);
	}

	/*
	 * To later detect whether a journal commit for sync writes is
	 * necessary, we sample i_size, and cluster count here.
	 */
	old_size = i_size_read(inode);
	old_clusters = OCFS2_I(inode)->ip_clusters;

	/* communicate with ocfs2_dio_end_io */
	ocfs2_iocb_set_rw_locked(iocb, rw_level);

	written = __generic_file_write_iter(iocb, from);
	/* buffered aio wouldn't have proper lock coverage today */
	BUG_ON(written == -EIOCBQUEUED && !(iocb->ki_flags & IOCB_DIRECT));

	if (unlikely(written <= 0))
		goto no_sync;

	if (((file->f_flags & O_DSYNC) && !direct_io) ||
	    IS_SYNC(inode) || dropped_dio) {
		ret = filemap_fdatawrite_range(file->f_mapping,
					       iocb->ki_pos - written,
					       iocb->ki_pos - 1);
		if (ret < 0)
			written = ret;

		if (!ret) {
			ret = jbd2_journal_force_commit(osb->journal->j_journal);
			if (ret < 0)
				written = ret;
		}

		if (!ret)
			ret = filemap_fdatawait_range(file->f_mapping,
						      iocb->ki_pos - written,
						      iocb->ki_pos - 1);
	}

no_sync:
	/*
	 * deep in g_f_a_w_n()->ocfs2_direct_IO we pass in a ocfs2_dio_end_io
	 * function pointer which is called when o_direct io completes so that
	 * it can unlock our rw lock.
	 * Unfortunately there are error cases which call end_io and others
	 * that don't.  so we don't have to unlock the rw_lock if either an
	 * async dio is going to do it in the future or an end_io after an
	 * error has already done it.
	 */
	if ((ret == -EIOCBQUEUED) || (!ocfs2_iocb_is_rw_locked(iocb))) {
		rw_level = -1;
		have_alloc_sem = 0;
		unaligned_dio = 0;
	}

	if (unaligned_dio) {
		ocfs2_iocb_clear_unaligned_aio(iocb);
		mutex_unlock(&OCFS2_I(inode)->ip_unaligned_aio);
	}

out:
	if (rw_level != -1)
		ocfs2_rw_unlock(inode, rw_level);

out_sems:
	if (have_alloc_sem)
		ocfs2_iocb_clear_sem_locked(iocb);

	mutex_unlock(&inode->i_mutex);

	if (written)
		ret = written;
	return ret;
}
