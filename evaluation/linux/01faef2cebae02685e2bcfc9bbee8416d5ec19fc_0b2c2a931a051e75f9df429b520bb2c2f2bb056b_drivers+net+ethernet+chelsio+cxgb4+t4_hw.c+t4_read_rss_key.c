void t4_read_rss_key(struct adapter *adap, u32 *key)
{
	if (t4_use_ldst(adap))
		t4_fw_tp_pio_rw(adap, key, 10, TP_RSS_SECRET_KEY0_A, 1);
	else
		t4_read_indirect(adap, TP_PIO_ADDR_A, TP_PIO_DATA_A, key, 10,
				 TP_RSS_SECRET_KEY0_A);
}
