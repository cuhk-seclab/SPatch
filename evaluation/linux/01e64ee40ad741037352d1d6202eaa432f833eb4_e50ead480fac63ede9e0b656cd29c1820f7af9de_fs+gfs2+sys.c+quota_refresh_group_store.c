static ssize_t quota_refresh_group_store(struct gfs2_sbd *sdp, const char *buf,
					 size_t len)
{
	struct kqid qid;
	int error;
	u32 id;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;

	error = kstrtou32(buf, 0, &id);
	if (error)
		return error;

	qid = make_kqid(current_user_ns(), GRPQUOTA, id);
	if (!qid_valid(qid))
		return -EINVAL;

	error = gfs2_quota_refresh(sdp, qid);
	return error ? error : len;
}
