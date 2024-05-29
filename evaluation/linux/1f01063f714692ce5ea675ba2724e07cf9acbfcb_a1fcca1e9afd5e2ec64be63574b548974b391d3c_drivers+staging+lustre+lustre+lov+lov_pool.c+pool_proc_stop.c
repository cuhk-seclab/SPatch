static void pool_proc_stop(struct seq_file *s, void *v)
{
	struct pool_iterator *iter = (struct pool_iterator *)s->private;

	/* in some cases stop() method is called 2 times, without
	 * calling start() method (see seq_read() from fs/seq_file.c)
	 * we have to free only if s->private is an iterator */
	if ((iter) && (iter->magic == POOL_IT_MAGIC)) {
		/* we restore s->private so next call to pool_proc_start()
		 * will work */
		s->private = iter->pool;
		lov_pool_putref(iter->pool);
		kfree(iter);
	}
}
