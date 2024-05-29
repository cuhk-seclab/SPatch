static void endio(struct bio *bio)
{
	struct io *io;
	unsigned region;
	int error;

	if (bio->bi_error && bio_data_dir(bio) == READ)
		zero_fill_bio(bio);

	/*
	 * The bio destructor in bio_put() may use the io object.
	 */
	retrieve_io_and_region_from_bio(bio, &io, &region);

	error = bio->bi_error;
	bio_put(bio);

	dec_count(io, region, error);
}
