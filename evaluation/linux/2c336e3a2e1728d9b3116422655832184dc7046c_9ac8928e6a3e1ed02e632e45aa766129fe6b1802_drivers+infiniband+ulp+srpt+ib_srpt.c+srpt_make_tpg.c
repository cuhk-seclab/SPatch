static struct se_portal_group *srpt_make_tpg(struct se_wwn *wwn,
					     struct config_group *group,
					     const char *name)
{
	struct srpt_port *sport = container_of(wwn, struct srpt_port, port_wwn);
	int res;

	/* Initialize sport->port_wwn and sport->port_tpg_1 */
	res = core_tpg_register(&srpt_target->tf_ops, &sport->port_wwn,
			&sport->port_tpg_1, sport, TRANSPORT_TPG_TYPE_NORMAL);
	if (res)
		return ERR_PTR(res);

	return &sport->port_tpg_1;
}
