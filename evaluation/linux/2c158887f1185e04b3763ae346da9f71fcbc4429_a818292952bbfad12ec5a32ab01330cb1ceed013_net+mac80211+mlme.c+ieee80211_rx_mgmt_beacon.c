static void ieee80211_rx_mgmt_beacon(struct ieee80211_sub_if_data *sdata,
				     struct ieee80211_mgmt *mgmt, size_t len,
				     struct ieee80211_rx_status *rx_status)
{
	struct ieee80211_if_managed *ifmgd = &sdata->u.mgd;
	struct ieee80211_bss_conf *bss_conf = &sdata->vif.bss_conf;
	size_t baselen;
	struct ieee802_11_elems elems;
	struct ieee80211_local *local = sdata->local;
	struct ieee80211_chanctx_conf *chanctx_conf;
	struct ieee80211_channel *chan;
	struct sta_info *sta;
	u32 changed = 0;
	bool erp_valid;
	u8 erp_value = 0;
	u32 ncrc;
	u8 *bssid;
	u8 deauth_buf[IEEE80211_DEAUTH_FRAME_LEN];

	sdata_assert_lock(sdata);

	/* Process beacon from the current BSS */
	baselen = (u8 *) mgmt->u.beacon.variable - (u8 *) mgmt;
	if (baselen > len)
		return;

	rcu_read_lock();
	chanctx_conf = rcu_dereference(sdata->vif.chanctx_conf);
	if (!chanctx_conf) {
		rcu_read_unlock();
		return;
	}

	if (rx_status->freq != chanctx_conf->def.chan->center_freq) {
		rcu_read_unlock();
		return;
	}
	chan = chanctx_conf->def.chan;
	rcu_read_unlock();

	if (ifmgd->assoc_data && ifmgd->assoc_data->need_beacon &&
	    ether_addr_equal(mgmt->bssid, ifmgd->assoc_data->bss->bssid)) {
		ieee802_11_parse_elems(mgmt->u.beacon.variable,
				       len - baselen, false, &elems);

		ieee80211_rx_bss_info(sdata, mgmt, len, rx_status, &elems);
		if (elems.tim && !elems.parse_error) {
			const struct ieee80211_tim_ie *tim_ie = elems.tim;
			ifmgd->dtim_period = tim_ie->dtim_period;
		}
		ifmgd->have_beacon = true;
		ifmgd->assoc_data->need_beacon = false;
		if (local->hw.flags & IEEE80211_HW_TIMING_BEACON_ONLY) {
			sdata->vif.bss_conf.sync_tsf =
				le64_to_cpu(mgmt->u.beacon.timestamp);
			sdata->vif.bss_conf.sync_device_ts =
				rx_status->device_timestamp;
			if (elems.tim)
				sdata->vif.bss_conf.sync_dtim_count =
					elems.tim->dtim_count;
			else
				sdata->vif.bss_conf.sync_dtim_count = 0;
		}
		/* continue assoc process */
		ifmgd->assoc_data->timeout = jiffies;
		ifmgd->assoc_data->timeout_started = true;
		run_again(sdata, ifmgd->assoc_data->timeout);
		return;
	}

	if (!ifmgd->associated ||
	    !ether_addr_equal(mgmt->bssid, ifmgd->associated->bssid))
		return;
	bssid = ifmgd->associated->bssid;

	/* Track average RSSI from the Beacon frames of the current AP */
	ifmgd->last_beacon_signal = rx_status->signal;
	if (ifmgd->flags & IEEE80211_STA_RESET_SIGNAL_AVE) {
		ifmgd->flags &= ~IEEE80211_STA_RESET_SIGNAL_AVE;
		ifmgd->ave_beacon_signal = rx_status->signal * 16;
		ifmgd->last_cqm_event_signal = 0;
		ifmgd->count_beacon_signal = 1;
		ifmgd->last_ave_beacon_signal = 0;
	} else {
		ifmgd->ave_beacon_signal =
			(IEEE80211_SIGNAL_AVE_WEIGHT * rx_status->signal * 16 +
			 (16 - IEEE80211_SIGNAL_AVE_WEIGHT) *
			 ifmgd->ave_beacon_signal) / 16;
		ifmgd->count_beacon_signal++;
	}

	if (ifmgd->rssi_min_thold != ifmgd->rssi_max_thold &&
	    ifmgd->count_beacon_signal >= IEEE80211_SIGNAL_AVE_MIN_COUNT) {
		int sig = ifmgd->ave_beacon_signal;
		int last_sig = ifmgd->last_ave_beacon_signal;

		/*
		 * if signal crosses either of the boundaries, invoke callback
		 * with appropriate parameters
		 */
		if (sig > ifmgd->rssi_max_thold &&
		    (last_sig <= ifmgd->rssi_min_thold || last_sig == 0)) {
			ifmgd->last_ave_beacon_signal = sig;
			drv_rssi_callback(local, sdata, RSSI_EVENT_HIGH);
		} else if (sig < ifmgd->rssi_min_thold &&
			   (last_sig >= ifmgd->rssi_max_thold ||
			   last_sig == 0)) {
			ifmgd->last_ave_beacon_signal = sig;
			drv_rssi_callback(local, sdata, RSSI_EVENT_LOW);
		}
	}

	if (bss_conf->cqm_rssi_thold &&
	    ifmgd->count_beacon_signal >= IEEE80211_SIGNAL_AVE_MIN_COUNT &&
	    !(sdata->vif.driver_flags & IEEE80211_VIF_SUPPORTS_CQM_RSSI)) {
		int sig = ifmgd->ave_beacon_signal / 16;
		int last_event = ifmgd->last_cqm_event_signal;
		int thold = bss_conf->cqm_rssi_thold;
		int hyst = bss_conf->cqm_rssi_hyst;
		if (sig < thold &&
		    (last_event == 0 || sig < last_event - hyst)) {
			ifmgd->last_cqm_event_signal = sig;
			ieee80211_cqm_rssi_notify(
				&sdata->vif,
				NL80211_CQM_RSSI_THRESHOLD_EVENT_LOW,
				GFP_KERNEL);
		} else if (sig > thold &&
			   (last_event == 0 || sig > last_event + hyst)) {
			ifmgd->last_cqm_event_signal = sig;
			ieee80211_cqm_rssi_notify(
				&sdata->vif,
				NL80211_CQM_RSSI_THRESHOLD_EVENT_HIGH,
				GFP_KERNEL);
		}
	}

	if (ifmgd->flags & IEEE80211_STA_CONNECTION_POLL) {
		mlme_dbg_ratelimited(sdata,
				     "cancelling AP probe due to a received beacon\n");
		ieee80211_reset_ap_probe(sdata);
	}

	/*
	 * Push the beacon loss detection into the future since
	 * we are processing a beacon from the AP just now.
	 */
	ieee80211_sta_reset_beacon_monitor(sdata);

	ncrc = crc32_be(0, (void *)&mgmt->u.beacon.beacon_int, 4);
	ncrc = ieee802_11_parse_elems_crc(mgmt->u.beacon.variable,
					  len - baselen, false, &elems,
					  care_about_ies, ncrc);

	if (local->hw.flags & IEEE80211_HW_PS_NULLFUNC_STACK) {
		bool directed_tim = ieee80211_check_tim(elems.tim,
							elems.tim_len,
							ifmgd->aid);
		if (directed_tim) {
			if (local->hw.conf.dynamic_ps_timeout > 0) {
				if (local->hw.conf.flags & IEEE80211_CONF_PS) {
					local->hw.conf.flags &= ~IEEE80211_CONF_PS;
					ieee80211_hw_config(local,
							    IEEE80211_CONF_CHANGE_PS);
				}
				ieee80211_send_nullfunc(local, sdata, 0);
			} else if (!local->pspolling && sdata->u.mgd.powersave) {
				local->pspolling = true;

				/*
				 * Here is assumed that the driver will be
				 * able to send ps-poll frame and receive a
				 * response even though power save mode is
				 * enabled, but some drivers might require
				 * to disable power save here. This needs
				 * to be investigated.
				 */
				ieee80211_send_pspoll(local, sdata);
			}
		}
	}

	if (sdata->vif.p2p) {
		struct ieee80211_p2p_noa_attr noa = {};
		int ret;

		ret = cfg80211_get_p2p_attr(mgmt->u.beacon.variable,
					    len - baselen,
					    IEEE80211_P2P_ATTR_ABSENCE_NOTICE,
					    (u8 *) &noa, sizeof(noa));
		if (ret >= 2) {
			if (sdata->u.mgd.p2p_noa_index != noa.index) {
				/* valid noa_attr and index changed */
				sdata->u.mgd.p2p_noa_index = noa.index;
				memcpy(&bss_conf->p2p_noa_attr, &noa, sizeof(noa));
				changed |= BSS_CHANGED_P2P_PS;
				/*
				 * make sure we update all information, the CRC
				 * mechanism doesn't look at P2P attributes.
				 */
				ifmgd->beacon_crc_valid = false;
			}
		} else if (sdata->u.mgd.p2p_noa_index != -1) {
			/* noa_attr not found and we had valid noa_attr before */
			sdata->u.mgd.p2p_noa_index = -1;
			memset(&bss_conf->p2p_noa_attr, 0, sizeof(bss_conf->p2p_noa_attr));
			changed |= BSS_CHANGED_P2P_PS;
			ifmgd->beacon_crc_valid = false;
		}
	}

	if (ifmgd->csa_waiting_bcn)
		ieee80211_chswitch_post_beacon(sdata);

	/*
	 * Update beacon timing and dtim count on every beacon appearance. This
	 * will allow the driver to use the most updated values. Do it before
	 * comparing this one with last received beacon.
	 * IMPORTANT: These parameters would possibly be out of sync by the time
	 * the driver will use them. The synchronized view is currently
	 * guaranteed only in certain callbacks.
	 */
	if (local->hw.flags & IEEE80211_HW_TIMING_BEACON_ONLY) {
		sdata->vif.bss_conf.sync_tsf =
			le64_to_cpu(mgmt->u.beacon.timestamp);
		sdata->vif.bss_conf.sync_device_ts =
			rx_status->device_timestamp;
		if (elems.tim)
			sdata->vif.bss_conf.sync_dtim_count =
				elems.tim->dtim_count;
		else
			sdata->vif.bss_conf.sync_dtim_count = 0;
	}

	if (ncrc == ifmgd->beacon_crc && ifmgd->beacon_crc_valid)
		return;
	ifmgd->beacon_crc = ncrc;
	ifmgd->beacon_crc_valid = true;

	ieee80211_rx_bss_info(sdata, mgmt, len, rx_status, &elems);

	ieee80211_sta_process_chanswitch(sdata, rx_status->mactime,
					 rx_status->device_timestamp,
					 &elems, true);

	if (!(ifmgd->flags & IEEE80211_STA_DISABLE_WMM) &&
	    ieee80211_sta_wmm_params(local, sdata, elems.wmm_param,
				     elems.wmm_param_len))
		changed |= BSS_CHANGED_QOS;

	/*
	 * If we haven't had a beacon before, tell the driver about the
	 * DTIM period (and beacon timing if desired) now.
	 */
	if (!ifmgd->have_beacon) {
		/* a few bogus AP send dtim_period = 0 or no TIM IE */
		if (elems.tim)
			bss_conf->dtim_period = elems.tim->dtim_period ?: 1;
		else
			bss_conf->dtim_period = 1;

		changed |= BSS_CHANGED_BEACON_INFO;
		ifmgd->have_beacon = true;

		mutex_lock(&local->iflist_mtx);
		ieee80211_recalc_ps(local, -1);
		mutex_unlock(&local->iflist_mtx);

		ieee80211_recalc_ps_vif(sdata);
	}

	if (elems.erp_info) {
		erp_valid = true;
		erp_value = elems.erp_info[0];
	} else {
		erp_valid = false;
	}
	changed |= ieee80211_handle_bss_capability(sdata,
			le16_to_cpu(mgmt->u.beacon.capab_info),
			erp_valid, erp_value);

	mutex_lock(&local->sta_mtx);
	sta = sta_info_get(sdata, bssid);

	if (ieee80211_config_bw(sdata, sta,
				elems.ht_cap_elem, elems.ht_operation,
				elems.vht_operation, bssid, &changed)) {
		mutex_unlock(&local->sta_mtx);
		ieee80211_set_disassoc(sdata, IEEE80211_STYPE_DEAUTH,
				       WLAN_REASON_DEAUTH_LEAVING,
				       true, deauth_buf);
		cfg80211_tx_mlme_mgmt(sdata->dev, deauth_buf,
				      sizeof(deauth_buf));
		return;
	}

	if (sta && elems.opmode_notif)
		ieee80211_vht_handle_opmode(sdata, sta, *elems.opmode_notif,
					    rx_status->band, true);
	mutex_unlock(&local->sta_mtx);

	changed |= ieee80211_handle_pwr_constr(sdata, chan, mgmt,
					       elems.country_elem,
					       elems.country_elem_len,
					       elems.pwr_constr_elem,
					       elems.cisco_dtpc_elem);

	ieee80211_bss_info_change_notify(sdata, changed);
}
