static int persistent_ctr(struct dm_exception_store *store, char *options)
{
	struct pstore *ps;

	/* allocate the pstore */
	ps = kzalloc(sizeof(*ps), GFP_KERNEL);
	if (!ps)
		return -ENOMEM;

	ps->store = store;
	ps->valid = 1;
	ps->version = SNAPSHOT_DISK_VERSION;
	ps->area = NULL;
	ps->zero_area = NULL;
	ps->header_area = NULL;
	ps->next_free = NUM_SNAPSHOT_HDR_CHUNKS + 1; /* header and 1st area */
	ps->current_committed = 0;

	ps->callback_count = 0;
	atomic_set(&ps->pending_count, 0);
	ps->callbacks = NULL;

	ps->metadata_wq = alloc_workqueue("ksnaphd", WQ_MEM_RECLAIM, 0);
	if (!ps->metadata_wq) {
		kfree(ps);
		DMERR("couldn't start header metadata update thread");
		return -ENOMEM;
	}

	if (options) {
		char overflow = toupper(options[0]);
		if (overflow == 'O')
			store->userspace_supports_overflow = true;
		else {
			DMERR("Unsupported persistent store option: %s", options);
			return -EINVAL;
		}
	}

	store->context = ps;

	return 0;
}
