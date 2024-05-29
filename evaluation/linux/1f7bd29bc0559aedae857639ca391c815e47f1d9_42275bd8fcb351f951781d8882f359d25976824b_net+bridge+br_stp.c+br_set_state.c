void br_set_state(struct net_bridge_port *p, unsigned int state)
{
	struct switchdev_attr attr = {
		.id = SWITCHDEV_ATTR_PORT_STP_STATE,
		.stp_state = state,
	};
	int err;

	p->state = state;
	err = switchdev_port_attr_set(p->dev, &attr);
	if (err && err != -EOPNOTSUPP)
		br_warn(p->br, "error setting offload STP state on port %u(%s)\n",
				(unsigned int) p->port_no, p->dev->name);
}
