static ssize_t fuse_dev_do_read(struct fuse_conn *fc, struct file *file,
				struct fuse_copy_state *cs, size_t nbytes)
{
	ssize_t err;
	struct fuse_iqueue *fiq = &fc->iq;
	struct fuse_pqueue *fpq = &fc->pq;
	struct fuse_req *req;
	struct fuse_in *in;
	unsigned reqsize;

 restart:
	spin_lock(&fiq->waitq.lock);
	err = -EAGAIN;
	if ((file->f_flags & O_NONBLOCK) && fiq->connected &&
	    !request_pending(fiq))
		goto err_unlock;

	err = wait_event_interruptible_exclusive_locked(fiq->waitq,
				!fiq->connected || request_pending(fiq));
	if (err)
		goto err_unlock;

	err = -ENODEV;
	if (!fiq->connected)
		goto err_unlock;

	if (!list_empty(&fiq->interrupts)) {
		req = list_entry(fiq->interrupts.next, struct fuse_req,
				 intr_entry);
		return fuse_read_interrupt(fiq, cs, nbytes, req);
	}

	if (forget_pending(fiq)) {
		if (list_empty(&fiq->pending) || fiq->forget_batch-- > 0)
			return fuse_read_forget(fc, fiq, cs, nbytes);

		if (fiq->forget_batch <= -8)
			fiq->forget_batch = 16;
	}

	req = list_entry(fiq->pending.next, struct fuse_req, list);
	clear_bit(FR_PENDING, &req->flags);
	list_del_init(&req->list);
	spin_unlock(&fiq->waitq.lock);

	spin_lock(&fc->lock);
	in = &req->in;
	reqsize = in->h.len;
	/* If request is too large, reply with an error and restart the read */
	if (nbytes < reqsize) {
		req->out.h.error = -EIO;
		/* SETXATTR is special, since it may contain too large data */
		if (in->h.opcode == FUSE_SETXATTR)
			req->out.h.error = -E2BIG;
		spin_unlock(&fc->lock);
		request_end(fc, req);
		goto restart;
	}
	spin_lock(&fpq->lock);
	list_add(&req->list, &fpq->io);
	spin_unlock(&fpq->lock);
	spin_unlock(&fc->lock);
	cs->req = req;
	err = fuse_copy_one(cs, &in->h, sizeof(in->h));
	if (!err)
		err = fuse_copy_args(cs, in->numargs, in->argpages,
				     (struct fuse_arg *) in->args, 0);
	fuse_copy_finish(cs);
	spin_lock(&fc->lock);
	spin_lock(&fpq->lock);
	clear_bit(FR_LOCKED, &req->flags);
	if (!fpq->connected) {
		err = -ENODEV;
		goto out_end;
	}
	if (err) {
		req->out.h.error = -EIO;
		goto out_end;
	}
	if (!test_bit(FR_ISREPLY, &req->flags)) {
		err = reqsize;
		goto out_end;
	}
	list_move_tail(&req->list, &fpq->processing);
	spin_unlock(&fpq->lock);
	set_bit(FR_SENT, &req->flags);
	/* matches barrier in request_wait_answer() */
	smp_mb__after_atomic();
	if (test_bit(FR_INTERRUPTED, &req->flags))
		queue_interrupt(fiq, req);
	spin_unlock(&fc->lock);

	return reqsize;

out_end:
	if (!test_bit(FR_PRIVATE, &req->flags))
		list_del_init(&req->list);
	spin_unlock(&fpq->lock);
	spin_unlock(&fc->lock);
	request_end(fc, req);
	return err;

 err_unlock:
	spin_unlock(&fiq->waitq.lock);
	return err;
}
