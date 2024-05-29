static int _rtl8821ae_set_media_status(struct ieee80211_hw *hw,
				     enum nl80211_iftype type)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 bt_msr = rtl_read_byte(rtlpriv, MSR);
	enum led_ctl_mode ledaction = LED_CTL_NO_LINK;
	bt_msr &= 0xfc;

	rtl_write_dword(rtlpriv, REG_BCN_CTRL, 0);
	RT_TRACE(rtlpriv, COMP_BEACON, DBG_LOUD,
		"clear 0x550 when set HW_VAR_MEDIA_STATUS\n");

	if (type == NL80211_IFTYPE_UNSPECIFIED ||
	    type == NL80211_IFTYPE_STATION) {
		_rtl8821ae_stop_tx_beacon(hw);
		_rtl8821ae_enable_bcn_sub_func(hw);
	} else if (type == NL80211_IFTYPE_ADHOC ||
		type == NL80211_IFTYPE_AP) {
		_rtl8821ae_resume_tx_beacon(hw);
		_rtl8821ae_disable_bcn_sub_func(hw);
	} else {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
			 "Set HW_VAR_MEDIA_STATUS: No such media status(%x).\n",
			 type);
	}

	switch (type) {
	case NL80211_IFTYPE_UNSPECIFIED:
		bt_msr |= MSR_NOLINK;
		ledaction = LED_CTL_LINK;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to NO LINK!\n");
		break;
	case NL80211_IFTYPE_ADHOC:
		bt_msr |= MSR_ADHOC;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to Ad Hoc!\n");
		break;
	case NL80211_IFTYPE_STATION:
		bt_msr |= MSR_INFRA;
		ledaction = LED_CTL_LINK;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to STA!\n");
		break;
	case NL80211_IFTYPE_AP:
		bt_msr |= MSR_AP;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to AP!\n");
		break;
	default:
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Network type %d not support!\n", type);
		return 1;
	}

	rtl_write_byte(rtlpriv, MSR, bt_msr);
	rtlpriv->cfg->ops->led_control(hw, ledaction);
	if ((bt_msr & 0xfc) == MSR_AP)
		rtl_write_byte(rtlpriv, REG_BCNTCFG + 1, 0x00);
	else
		rtl_write_byte(rtlpriv, REG_BCNTCFG + 1, 0x66);

	return 0;
}
