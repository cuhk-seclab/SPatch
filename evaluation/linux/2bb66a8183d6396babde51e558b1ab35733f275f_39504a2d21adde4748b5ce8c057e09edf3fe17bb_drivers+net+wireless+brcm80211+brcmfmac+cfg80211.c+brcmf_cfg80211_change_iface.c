static s32
brcmf_cfg80211_change_iface(struct wiphy *wiphy, struct net_device *ndev,
			 enum nl80211_iftype type, u32 *flags,
			 struct vif_params *params)
{
	struct brcmf_cfg80211_info *cfg = wiphy_priv(wiphy);
	struct brcmf_if *ifp = netdev_priv(ndev);
	struct brcmf_cfg80211_vif *vif = ifp->vif;
	s32 infra = 0;
	s32 ap = 0;
	s32 err = 0;

	brcmf_dbg(TRACE, "Enter, idx=%d, type=%d\n", ifp->bssidx, type);
	err = brcmf_vif_change_validate(wiphy_to_cfg(wiphy), vif, type);
	if (err) {
		brcmf_err("iface validation failed: err=%d\n", err);
		return err;
	}
	switch (type) {
	case NL80211_IFTYPE_MONITOR:
	case NL80211_IFTYPE_WDS:
		brcmf_err("type (%d) : currently we do not support this type\n",
			  type);
		return -EOPNOTSUPP;
	case NL80211_IFTYPE_ADHOC:
		infra = 0;
		break;
	case NL80211_IFTYPE_STATION:
		/* Ignore change for p2p IF. Unclear why supplicant does this */
		if ((vif->wdev.iftype == NL80211_IFTYPE_P2P_CLIENT) ||
		    (vif->wdev.iftype == NL80211_IFTYPE_P2P_GO)) {
			brcmf_dbg(TRACE, "Ignoring cmd for p2p if\n");
			/* WAR: It is unexpected to get a change of VIF for P2P
			 * IF, but it happens. The request can not be handled
			 * but returning EPERM causes a crash. Returning 0
			 * without setting ieee80211_ptr->iftype causes trace
			 * (WARN_ON) but it works with wpa_supplicant
			 */
			return 0;
		}
		infra = 1;
		break;
	case NL80211_IFTYPE_AP:
	case NL80211_IFTYPE_P2P_GO:
		ap = 1;
		break;
	default:
		err = -EINVAL;
		goto done;
	}

	if (ap) {
		if (type == NL80211_IFTYPE_P2P_GO) {
			brcmf_dbg(INFO, "IF Type = P2P GO\n");
			err = brcmf_p2p_ifchange(cfg, BRCMF_FIL_P2P_IF_GO);
		}
		if (!err) {
			set_bit(BRCMF_VIF_STATUS_AP_CREATING, &vif->sme_state);
			brcmf_dbg(INFO, "IF Type = AP\n");
		}
	} else {
		err = brcmf_fil_cmd_int_set(ifp, BRCMF_C_SET_INFRA, infra);
		if (err) {
			brcmf_err("WLC_SET_INFRA error (%d)\n", err);
			err = -EAGAIN;
			goto done;
		}
		brcmf_dbg(INFO, "IF Type = %s\n", brcmf_is_ibssmode(vif) ?
			  "Adhoc" : "Infra");
	}
	ndev->ieee80211_ptr->iftype = type;

	brcmf_cfg80211_update_proto_addr_mode(&vif->wdev);

done:
	brcmf_dbg(TRACE, "Exit\n");

	return err;
}
