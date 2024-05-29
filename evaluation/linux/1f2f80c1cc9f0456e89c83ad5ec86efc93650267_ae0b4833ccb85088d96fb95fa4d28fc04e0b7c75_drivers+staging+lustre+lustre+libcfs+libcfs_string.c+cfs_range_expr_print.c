static int
cfs_range_expr_print(char *buffer, int count, struct cfs_range_expr *expr,
		     bool bracketed)
{
	int i;
	char s[] = "[";
	char e[] = "]";

	if (bracketed)
		s[0] = e[0] = '\0';

	if (expr->re_lo == expr->re_hi)
		i = scnprintf(buffer, count, "%u", expr->re_lo);
	else if (expr->re_stride == 1)
		i = scnprintf(buffer, count, "%s%u-%u%s",
			      s, expr->re_lo, expr->re_hi, e);
	else
		i = scnprintf(buffer, count, "%s%u-%u/%u%s",
			      s, expr->re_lo, expr->re_hi, expr->re_stride, e);
	return i;
}
