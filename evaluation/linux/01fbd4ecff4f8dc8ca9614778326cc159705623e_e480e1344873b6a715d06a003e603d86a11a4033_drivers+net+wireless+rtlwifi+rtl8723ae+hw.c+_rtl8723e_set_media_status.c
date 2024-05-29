static int _rtl8723e_set_media_status(struct ieee80211_hw *hw,
				      enum nl80211_iftype type)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 bt_msr = rtl_read_byte(rtlpriv, MSR) & 0xfc;
	enum led_ctl_mode ledaction = LED_CTL_NO_LINK;
	u8 mode = MSR_NOLINK;

	rtl_write_dword(rtlpriv, REG_BCN_CTRL, 0);
	RT_TRACE(rtlpriv, COMP_BEACON, DBG_LOUD,
		"clear 0x550 when set HW_VAR_MEDIA_STATUS\n");

	switch (type) {
	case NL80211_IFTYPE_UNSPECIFIED:
		mode = MSR_NOLINK;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			"Set Network type to NO LINK!\n");
		break;
	case NL80211_IFTYPE_ADHOC:
		mode = MSR_ADHOC;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			"Set Network type to Ad Hoc!\n");
		break;
	case NL80211_IFTYPE_STATION:
		mode = MSR_INFRA;
		ledaction = LED_CTL_LINK;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			"Set Network type to STA!\n");
		break;
	case NL80211_IFTYPE_AP:
		mode = MSR_AP;
		ledaction = LED_CTL_LINK;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			"Set Network type to AP!\n");
		break;
	default:
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			"Network type %d not support!\n", type);
		return 1;
		break;
	}

	/* MSR_INFRA == Link in infrastructure network;
	 * MSR_ADHOC == Link in ad hoc network;
	 * Therefore, check link state is necessary.
	 *
	 * MSR_AP == AP mode; link state is not cared here.
	 */
	if (mode != MSR_AP &&
	    rtlpriv->mac80211.link_state < MAC80211_LINKED) {
		mode = MSR_NOLINK;
		ledaction = LED_CTL_NO_LINK;
	}
	if (mode == MSR_NOLINK || mode == MSR_INFRA) {
		_rtl8723e_stop_tx_beacon(hw);
		_rtl8723e_enable_bcn_sub_func(hw);
	} else if (mode == MSR_ADHOC || mode == MSR_AP) {
		_rtl8723e_resume_tx_beacon(hw);
		_rtl8723e_disable_bcn_sub_func(hw);
	} else {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
			 "Set HW_VAR_MEDIA_STATUS: No such media status(%x).\n",
			 mode);
	}

	rtl_write_byte(rtlpriv, MSR, bt_msr | mode);
	rtlpriv->cfg->ops->led_control(hw, ledaction);
	if (mode == MSR_AP)
		rtl_write_byte(rtlpriv, REG_BCNTCFG + 1, 0x00);
	else
		rtl_write_byte(rtlpriv, REG_BCNTCFG + 1, 0x66);
	return 0;
}
