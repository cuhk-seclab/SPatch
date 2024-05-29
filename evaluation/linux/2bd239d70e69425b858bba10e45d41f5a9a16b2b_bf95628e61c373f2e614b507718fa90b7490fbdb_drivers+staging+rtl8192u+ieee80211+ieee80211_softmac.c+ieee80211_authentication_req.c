inline struct sk_buff *ieee80211_authentication_req(struct ieee80211_network *beacon,
	struct ieee80211_device *ieee, int challengelen)
{
	struct sk_buff *skb;
	struct ieee80211_authentication *auth;
	int len = sizeof(struct ieee80211_authentication) + challengelen + ieee->tx_headroom;


	skb = dev_alloc_skb(len);
	if (!skb) return NULL;

	skb_reserve(skb, ieee->tx_headroom);
	auth = (struct ieee80211_authentication *)
		skb_put(skb, sizeof(struct ieee80211_authentication));

	if (challengelen)
		auth->header.frame_ctl = cpu_to_le16(IEEE80211_STYPE_AUTH
				| IEEE80211_FCTL_WEP);
	else
		auth->header.frame_ctl = cpu_to_le16(IEEE80211_STYPE_AUTH);

	auth->header.duration_id = 0x013a; //FIXME

	memcpy(auth->header.addr1, beacon->bssid, ETH_ALEN);
	memcpy(auth->header.addr2, ieee->dev->dev_addr, ETH_ALEN);
	memcpy(auth->header.addr3, beacon->bssid, ETH_ALEN);

	//auth->algorithm = ieee->open_wep ? WLAN_AUTH_OPEN : WLAN_AUTH_SHARED_KEY;
	if(ieee->auth_mode == 0)
		auth->algorithm = WLAN_AUTH_OPEN;
	else if(ieee->auth_mode == 1)
		auth->algorithm = WLAN_AUTH_SHARED_KEY;
	else if(ieee->auth_mode == 2)
		auth->algorithm = WLAN_AUTH_OPEN;//0x80;
	printk("=================>%s():auth->algorithm is %d\n",__func__,auth->algorithm);
	auth->transaction = cpu_to_le16(ieee->associate_seq);
	ieee->associate_seq++;

	auth->status = cpu_to_le16(WLAN_STATUS_SUCCESS);

	return skb;

}
