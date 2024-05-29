static int snapshot_map(struct dm_target *ti, struct bio *bio)
{
	struct dm_exception *e;
	struct dm_snapshot *s = ti->private;
	int r = DM_MAPIO_REMAPPED;
	chunk_t chunk;
	struct dm_snap_pending_exception *pe = NULL;

	init_tracked_chunk(bio);

	if (bio->bi_rw & REQ_FLUSH) {
		bio->bi_bdev = s->cow->bdev;
		return DM_MAPIO_REMAPPED;
	}

	chunk = sector_to_chunk(s->store, bio->bi_iter.bi_sector);

	/* Full snapshots are not usable */
	/* To get here the table must be live so s->active is always set. */
	if (!s->valid)
		return -EIO;

	/* FIXME: should only take write lock if we need
	 * to copy an exception */
	down_write(&s->lock);

	if (!s->valid || (unlikely(s->snapshot_overflowed) && bio_rw(bio) == WRITE)) {
		r = -EIO;
		goto out_unlock;
	}

	/* If the block is already remapped - use that, else remap it */
	e = dm_lookup_exception(&s->complete, chunk);
	if (e) {
		remap_exception(s, e, bio, chunk);
		goto out_unlock;
	}

	/*
	 * Write to snapshot - higher level takes care of RW/RO
	 * flags so we should only get this if we are
	 * writeable.
	 */
	if (bio_rw(bio) == WRITE) {
		pe = __lookup_pending_exception(s, chunk);
		if (!pe) {
			up_write(&s->lock);
			pe = alloc_pending_exception(s);
			down_write(&s->lock);

			if (!s->valid || s->snapshot_overflowed) {
				free_pending_exception(pe);
				r = -EIO;
				goto out_unlock;
			}

			e = dm_lookup_exception(&s->complete, chunk);
			if (e) {
				free_pending_exception(pe);
				remap_exception(s, e, bio, chunk);
				goto out_unlock;
			}

			pe = __find_pending_exception(s, pe, chunk);
			if (!pe) {
				if (s->store->userspace_supports_overflow) {
					s->snapshot_overflowed = 1;
					DMERR("Snapshot overflowed: Unable to allocate exception.");
				} else
					__invalidate_snapshot(s, -ENOMEM);
				r = -EIO;
				goto out_unlock;
			}
		}

		remap_exception(s, &pe->e, bio, chunk);

		r = DM_MAPIO_SUBMITTED;

		if (!pe->started &&
		    bio->bi_iter.bi_size ==
		    (s->store->chunk_size << SECTOR_SHIFT)) {
			pe->started = 1;
			up_write(&s->lock);
			start_full_bio(pe, bio);
			goto out;
		}

		bio_list_add(&pe->snapshot_bios, bio);

		if (!pe->started) {
			/* this is protected by snap->lock */
			pe->started = 1;
			up_write(&s->lock);
			start_copy(pe);
			goto out;
		}
	} else {
		bio->bi_bdev = s->origin->bdev;
		track_chunk(s, bio, chunk);
	}

out_unlock:
	up_write(&s->lock);
out:
	return r;
}
