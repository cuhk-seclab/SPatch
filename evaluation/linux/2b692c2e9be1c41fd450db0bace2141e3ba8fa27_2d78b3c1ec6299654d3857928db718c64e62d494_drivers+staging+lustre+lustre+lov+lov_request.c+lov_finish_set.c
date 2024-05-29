void lov_finish_set(struct lov_request_set *set)
{
	struct list_head *pos, *n;

	LASSERT(set);
	list_for_each_safe(pos, n, &set->set_list) {
		struct lov_request *req = list_entry(pos,
							 struct lov_request,
							 rq_link);
		list_del_init(&req->rq_link);

		if (req->rq_oi.oi_oa)
			OBDO_FREE(req->rq_oi.oi_oa);
		kfree(req->rq_oi.oi_osfs);
		kfree(req);
	}
	kfree(set);
}
