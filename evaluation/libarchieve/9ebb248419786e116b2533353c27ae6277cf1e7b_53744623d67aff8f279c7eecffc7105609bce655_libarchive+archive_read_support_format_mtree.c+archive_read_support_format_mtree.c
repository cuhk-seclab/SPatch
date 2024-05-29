int
archive_read_support_format_mtree(struct archive *_a)
{
	static const struct archive_rb_tree_ops rb_ops = {
		mtree_cmp_node, mtree_cmp_key,
	};
	struct archive_read *a = (struct archive_read *)_a;
	struct mtree *mtree;
	int r;

	archive_check_magic(_a, ARCHIVE_READ_MAGIC,
	    ARCHIVE_STATE_NEW, "archive_read_support_format_mtree");

	mtree = (struct mtree *)calloc(1, sizeof(*mtree));
	if (mtree == NULL) {
		archive_set_error(&a->archive, ENOMEM,
		    "Can't allocate mtree data");
		return (ARCHIVE_FATAL);
	}
	mtree->checkfs = 0;
	mtree->fd = -1;

	__archive_rb_tree_init(&mtree->rbtree, &rb_ops);

	r = __archive_read_register_format(a, mtree, "mtree",
           mtree_bid, archive_read_format_mtree_options, read_header, read_data, skip, NULL, cleanup, NULL, NULL);

	if (r != ARCHIVE_OK)
		free(mtree);
	return (ARCHIVE_OK);
}
