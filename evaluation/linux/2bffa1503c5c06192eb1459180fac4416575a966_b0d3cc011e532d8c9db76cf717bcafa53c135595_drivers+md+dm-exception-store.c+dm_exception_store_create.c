int dm_exception_store_create(struct dm_target *ti, int argc, char **argv,
			      struct dm_snapshot *snap,
			      unsigned *args_used,
			      struct dm_exception_store **store)
{
	int r = 0;
	struct dm_exception_store_type *type = NULL;
	struct dm_exception_store *tmp_store;
	char persistent;

	if (argc < 2) {
		ti->error = "Insufficient exception store arguments";
		return -EINVAL;
	}

	tmp_store = kzalloc(sizeof(*tmp_store), GFP_KERNEL);
	if (!tmp_store) {
		ti->error = "Exception store allocation failed";
		return -ENOMEM;
	}

	persistent = toupper(*argv[0]);
	if (persistent == 'P')
		type = get_type("P");
	else if (persistent == 'N')
		type = get_type("N");
	else {
		ti->error = "Exception store type is not P or N";
		r = -EINVAL;
		goto bad_type;
	}

	if (!type) {
		ti->error = "Exception store type not recognised";
		r = -EINVAL;
		goto bad_type;
	}

	tmp_store->type = type;
	tmp_store->snap = snap;

	r = set_chunk_size(tmp_store, argv[1], &ti->error);
	if (r)
		goto bad;

	r = type->ctr(tmp_store, (strlen(argv[0]) > 1 ? &argv[0][1] : NULL));
	if (r) {
		ti->error = "Exception store type constructor failed";
		goto bad;
	}

	*args_used = 2;
	*store = tmp_store;
	return 0;

bad:
	put_type(type);
bad_type:
	kfree(tmp_store);
	return r;
}
