static inline
void pnfs_fetch_commit_bucket_list(struct list_head *pages,
		struct nfs_commit_data *data,
		struct nfs_commit_info *cinfo)
{
	struct pnfs_commit_bucket *bucket;

	bucket = &cinfo->ds->buckets[data->ds_commit_index];
	spin_lock(cinfo->lock);
	list_splice_init(&bucket->committing, pages);
	data->lseg = bucket->clseg;
	bucket->clseg = NULL;
	spin_unlock(cinfo->lock);

}
