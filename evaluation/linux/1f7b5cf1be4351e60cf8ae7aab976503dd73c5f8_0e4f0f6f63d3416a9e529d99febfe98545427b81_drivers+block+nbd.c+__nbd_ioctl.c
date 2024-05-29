static int __nbd_ioctl(struct block_device *bdev, struct nbd_device *nbd,
		       unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	case NBD_DISCONNECT: {
		struct request sreq;

		dev_info(disk_to_dev(nbd->disk), "NBD_DISCONNECT\n");
		if (!nbd->sock)
			return -EINVAL;

		mutex_unlock(&nbd->tx_lock);
		fsync_bdev(bdev);
		mutex_lock(&nbd->tx_lock);
		blk_rq_init(NULL, &sreq);
		sreq.cmd_type = REQ_TYPE_DRV_PRIV;

		/* Check again after getting mutex back.  */
		if (!nbd->sock)
			return -EINVAL;

		nbd->disconnect = true;

		nbd_send_req(nbd, &sreq);
		return 0;
	}
 
	case NBD_CLEAR_SOCK:
		sock_shutdown(nbd);
		nbd_clear_que(nbd);
		BUG_ON(!list_empty(&nbd->queue_head));
		BUG_ON(!list_empty(&nbd->waiting_queue));
		kill_bdev(bdev);
		return 0;

	case NBD_SET_SOCK: {
		int err;
		struct socket *sock = sockfd_lookup(arg, &err);

		if (!sock)
			return err;

		err = nbd_set_socket(nbd, sock);
		if (!err && max_part)
			bdev->bd_invalidated = 1;

		return err;
	}

	case NBD_SET_BLKSIZE:
		nbd->blksize = arg;
		nbd->bytesize &= ~(nbd->blksize-1);
		bdev->bd_inode->i_size = nbd->bytesize;
		set_blocksize(bdev, nbd->blksize);
		set_capacity(nbd->disk, nbd->bytesize >> 9);
		return 0;

	case NBD_SET_SIZE:
		nbd->bytesize = arg & ~(nbd->blksize-1);
		bdev->bd_inode->i_size = nbd->bytesize;
		set_blocksize(bdev, nbd->blksize);
		set_capacity(nbd->disk, nbd->bytesize >> 9);
		return 0;

	case NBD_SET_TIMEOUT:
		nbd->xmit_timeout = arg * HZ;
		if (arg)
			mod_timer(&nbd->timeout_timer,
				  jiffies + nbd->xmit_timeout);
		else
			del_timer_sync(&nbd->timeout_timer);

		return 0;

	case NBD_SET_FLAGS:
		nbd->flags = arg;
		return 0;

	case NBD_SET_SIZE_BLOCKS:
		nbd->bytesize = ((u64) arg) * nbd->blksize;
		bdev->bd_inode->i_size = nbd->bytesize;
		set_blocksize(bdev, nbd->blksize);
		set_capacity(nbd->disk, nbd->bytesize >> 9);
		return 0;

	case NBD_DO_IT: {
		struct task_struct *thread;
		int error;

		if (nbd->task_recv)
			return -EBUSY;
		if (!nbd->sock)
			return -EINVAL;

		mutex_unlock(&nbd->tx_lock);

		if (nbd->flags & NBD_FLAG_READ_ONLY)
			set_device_ro(bdev, true);
		if (nbd->flags & NBD_FLAG_SEND_TRIM)
			queue_flag_set_unlocked(QUEUE_FLAG_DISCARD,
				nbd->disk->queue);
		if (nbd->flags & NBD_FLAG_SEND_FLUSH)
			blk_queue_flush(nbd->disk->queue, REQ_FLUSH);
		else
			blk_queue_flush(nbd->disk->queue, 0);

		thread = kthread_run(nbd_thread_send, nbd, "%s",
				     nbd_name(nbd));
		if (IS_ERR(thread)) {
			mutex_lock(&nbd->tx_lock);
			return PTR_ERR(thread);
		}

		nbd_dev_dbg_init(nbd);
		error = nbd_thread_recv(nbd);
		nbd_dev_dbg_close(nbd);
		kthread_stop(thread);

		mutex_lock(&nbd->tx_lock);

		sock_shutdown(nbd);
		nbd_clear_que(nbd);
		kill_bdev(bdev);
		nbd_bdev_reset(bdev);

		if (nbd->disconnect) /* user requested, ignore socket errors */
			error = 0;
		if (nbd->timedout)
			error = -ETIMEDOUT;

		nbd_reset(nbd);

		return error;
	}

	case NBD_CLEAR_QUE:
		/*
		 * This is for compatibility only.  The queue is always cleared
		 * by NBD_DO_IT or NBD_CLEAR_SOCK.
		 */
		return 0;

	case NBD_PRINT_DEBUG:
		dev_info(disk_to_dev(nbd->disk),
			"next = %p, prev = %p, head = %p\n",
			nbd->queue_head.next, nbd->queue_head.prev,
			&nbd->queue_head);
		return 0;
	}
	return -ENOTTY;
}
