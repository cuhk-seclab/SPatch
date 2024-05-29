static int add_port(struct mlx4_ib_dev *dev, int port_num, int slave)
{
	struct mlx4_port *p;
	int i;
	int ret;
	int is_eth = rdma_port_get_link_layer(&dev->ib_dev, port_num) ==
			IB_LINK_LAYER_ETHERNET;

	p = kzalloc(sizeof *p, GFP_KERNEL);
	if (!p)
		return -ENOMEM;

	p->dev = dev;
	p->port_num = port_num;
	p->slave = slave;

	ret = kobject_init_and_add(&p->kobj, &port_type,
				   kobject_get(dev->dev_ports_parent[slave]),
				   "%d", port_num);
	if (ret)
		goto err_alloc;

	p->pkey_group.name  = "pkey_idx";
	p->pkey_group.attrs =
		alloc_group_attrs(show_port_pkey,
				  is_eth ? NULL : store_port_pkey,
				  dev->dev->caps.pkey_table_len[port_num]);
	if (!p->pkey_group.attrs) {
		ret = -ENOMEM;
		goto err_alloc;
	}

	ret = sysfs_create_group(&p->kobj, &p->pkey_group);
	if (ret)
		goto err_free_pkey;

	p->gid_group.name  = "gid_idx";
	p->gid_group.attrs = alloc_group_attrs(show_port_gid_idx, NULL, 1);
	if (!p->gid_group.attrs) {
		ret = -ENOMEM;
		goto err_free_pkey;
	}

	ret = sysfs_create_group(&p->kobj, &p->gid_group);
	if (ret)
		goto err_free_gid;

	ret = add_vf_smi_entries(p);
	if (ret)
		goto err_free_gid;

	list_add_tail(&p->kobj.entry, &dev->pkeys.pkey_port_list[slave]);
	return 0;

err_free_gid:
	kfree(p->gid_group.attrs[0]);
	kfree(p->gid_group.attrs);

err_free_pkey:
	for (i = 0; i < dev->dev->caps.pkey_table_len[port_num]; ++i)
		kfree(p->pkey_group.attrs[i]);
	kfree(p->pkey_group.attrs);

err_alloc:
	kobject_put(dev->dev_ports_parent[slave]);
	kfree(p);
	return ret;
}
