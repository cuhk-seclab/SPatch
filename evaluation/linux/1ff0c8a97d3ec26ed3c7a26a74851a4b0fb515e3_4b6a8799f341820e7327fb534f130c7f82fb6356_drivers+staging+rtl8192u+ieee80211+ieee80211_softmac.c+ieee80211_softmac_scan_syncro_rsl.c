void ieee80211_softmac_scan_syncro(struct ieee80211_device *ieee)
{
	short ch = 0;
	u8 channel_map[MAX_CHANNEL_NUMBER+1];
	memcpy(channel_map, GET_DOT11D_INFO(ieee)->channel_map, MAX_CHANNEL_NUMBER+1);
	down(&ieee->scan_sem);

	while(1)
	{

		do{
			ch++;
			if (ch > MAX_CHANNEL_NUMBER)
				goto out; /* scan completed */
		}while(!channel_map[ch]);

		/* this function can be called in two situations
		 * 1- We have switched to ad-hoc mode and we are
		 *    performing a complete syncro scan before conclude
		 *    there are no interesting cell and to create a
		 *    new one. In this case the link state is
		 *    IEEE80211_NOLINK until we found an interesting cell.
		 *    If so the ieee8021_new_net, called by the RX path
		 *    will set the state to IEEE80211_LINKED, so we stop
		 *    scanning
		 * 2- We are linked and the root uses run iwlist scan.
		 *    So we switch to IEEE80211_LINKED_SCANNING to remember
		 *    that we are still logically linked (not interested in
		 *    new network events, despite for updating the net list,
		 *    but we are temporarly 'unlinked' as the driver shall
		 *    not filter RX frames and the channel is changing.
		 * So the only situation in witch are interested is to check
		 * if the state become LINKED because of the #1 situation
		 */

		if (ieee->state == IEEE80211_LINKED)
			goto out;
		ieee->set_chan(ieee->dev, ch);
		if(channel_map[ch] == 1)
		ieee80211_send_probe_requests(ieee);

		/* this prevent excessive time wait when we
		 * need to wait for a syncro scan to end..
		 */
		if(ieee->state < IEEE80211_LINKED)
		if (ieee->sync_scan_hurryup)
			goto out;

		msleep_interruptible_rsl(IEEE80211_SOFTMAC_SCAN_TIME);

	}
out:
	if(ieee->state < IEEE80211_LINKED){
		ieee->actscanning = false;
		up(&ieee->scan_sem);
	}
	else{
	ieee->sync_scan_hurryup = 0;
	if(IS_DOT11D_ENABLE(ieee))
		DOT11D_ScanComplete(ieee);
	up(&ieee->scan_sem);
}
}
