static void fdb_del_external_learn(struct net_bridge_fdb_entry *f)
{
	struct switchdev_obj obj = {
		.id = SWITCHDEV_OBJ_PORT_FDB,
		.u.fdb = {
			.addr = f->addr.addr,
			.vid = f->vlan_id,
		},
	};

	switchdev_port_obj_del(f->dst->dev, &obj);
}
