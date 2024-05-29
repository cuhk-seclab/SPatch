static struct gfs2_dirent *gfs2_init_dirent(struct inode *inode,
					    struct gfs2_dirent *dent,
					    const struct qstr *name,
					    struct buffer_head *bh)
{
	unsigned offset = 0;

	if (!gfs2_dirent_sentinel(dent))
		offset = GFS2_DIRENT_SIZE(be16_to_cpu(dent->de_name_len));
	return do_init_dirent(inode, dent, name, bh, offset);
}
