static int lov_io_fault_start(const struct lu_env *env,
			      const struct cl_io_slice *ios)
{
	struct cl_fault_io *fio;
	struct lov_io      *lio;
	struct lov_io_sub  *sub;

	fio = &ios->cis_io->u.ci_fault;
	lio = cl2lov_io(env, ios);
	sub = lov_sub_get(env, lio, lov_page_stripe(fio->ft_page));
	if (IS_ERR(sub))
		return PTR_ERR(sub);
	sub->sub_io->u.ci_fault.ft_nob = fio->ft_nob;
	lov_sub_put(sub);
	return lov_io_start(env, ios);
}
