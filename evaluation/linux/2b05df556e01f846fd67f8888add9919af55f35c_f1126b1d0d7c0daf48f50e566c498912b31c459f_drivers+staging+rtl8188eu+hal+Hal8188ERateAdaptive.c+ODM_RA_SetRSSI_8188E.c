void ODM_RA_SetRSSI_8188E(struct odm_dm_struct *dm_odm, u8 macid, u8 Rssi)
{
	struct odm_ra_info *pRaInfo = NULL;

	if ((NULL == dm_odm) || (macid >= ASSOCIATE_ENTRY_NUM))
		return;
	ODM_RT_TRACE(dm_odm, ODM_COMP_RATE_ADAPTIVE, ODM_DBG_TRACE,
		     (" macid =%d Rssi =%d\n", macid, Rssi));

	pRaInfo = &(dm_odm->RAInfo[macid]);
	pRaInfo->RssiStaRA = Rssi;
}
