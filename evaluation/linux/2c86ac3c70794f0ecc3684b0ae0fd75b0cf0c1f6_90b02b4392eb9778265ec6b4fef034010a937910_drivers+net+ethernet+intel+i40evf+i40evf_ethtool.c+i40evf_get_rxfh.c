static int i40evf_get_rxfh(struct net_device *netdev, u32 *indir, u8 *key,
			   u8 *hfunc)
{
	struct i40evf_adapter *adapter = netdev_priv(netdev);
	struct i40e_vsi *vsi = &adapter->vsi;
	u8 *seed = NULL, *lut;
	int ret;
	u16 i;

	if (hfunc)
		*hfunc = ETH_RSS_HASH_TOP;
	if (!indir)
		return 0;

	seed = key;

	lut = kzalloc(I40EVF_HLUT_ARRAY_SIZE, GFP_KERNEL);
	if (!lut)
		return -ENOMEM;

	ret = i40evf_get_rss(vsi, seed, lut, I40EVF_HLUT_ARRAY_SIZE);
	if (ret)
		goto out;

	/* Each 32 bits pointed by 'indir' is stored with a lut entry */
	for (i = 0; i < I40EVF_HLUT_ARRAY_SIZE; i++)
		indir[i] = (u32)lut[i];

out:
	kfree(lut);

	return ret;
}
