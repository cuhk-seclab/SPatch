static int __vlan_vid_del(struct net_device *dev, struct net_bridge *br,
			  u16 vid)
{
	const struct net_device_ops *ops = dev->netdev_ops;
	int err = 0;

	/* If driver uses VLAN ndo ops, use 8021q to delete vid
	 * on device, otherwise try switchdev ops to delete vid.
	 */

	if (ops->ndo_vlan_rx_kill_vid) {
		vlan_vid_del(dev, br->vlan_proto, vid);
	} else {
		struct switchdev_obj_port_vlan v = {
			.vid_begin = vid,
			.vid_end = vid,
		};

		err = switchdev_port_obj_del(dev, SWITCHDEV_OBJ_ID_PORT_VLAN,
					     &v);
		if (err == -EOPNOTSUPP)
			err = 0;
	}

	return err;
}
