static void swap_refcount_rec(void *a, void *b, int size)
{
	struct ocfs2_refcount_rec *l = a, *r = b, tmp;

	tmp = *l;
	*r = tmp;
}
