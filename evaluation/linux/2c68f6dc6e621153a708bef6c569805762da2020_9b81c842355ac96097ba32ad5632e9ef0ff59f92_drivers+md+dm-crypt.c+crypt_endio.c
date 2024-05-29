static void crypt_endio(struct bio *clone)
{
	struct dm_crypt_io *io = clone->bi_private;
	struct crypt_config *cc = io->cc;
	unsigned rw = bio_data_dir(clone);
	int error;

	/*
	 * free the processed pages
	 */
	if (rw == WRITE)
		crypt_free_buffer_pages(cc, clone);

	error = clone->bi_error;
	bio_put(clone);

	if (rw == READ && !error) {
		kcryptd_queue_crypt(io);
		return;
	}

	if (unlikely(error))
		io->error = error;

	crypt_dec_pending(io);
}
