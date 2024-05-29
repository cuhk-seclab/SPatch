static int
ldebugfs_fid_server_seq_show(struct seq_file *m, void *unused)
{
	struct lu_client_seq *seq = (struct lu_client_seq *)m->private;
	struct client_obd *cli;

	if (seq->lcs_exp) {
		cli = &seq->lcs_exp->exp_obd->u.cli;
		seq_printf(m, "%s\n", cli->cl_target_uuid.uuid);
	}

	return 0;
}
