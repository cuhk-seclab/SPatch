static int _rtl92de_set_media_status(struct ieee80211_hw *hw,
				     enum nl80211_iftype type)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 bt_msr = rtl_read_byte(rtlpriv, MSR);
	enum led_ctl_mode ledaction = LED_CTL_NO_LINK;
	u8 bcnfunc_enable;

	bt_msr &= 0xfc;

	if (type == NL80211_IFTYPE_UNSPECIFIED ||
	    type == NL80211_IFTYPE_STATION) {
		_rtl92de_stop_tx_beacon(hw);
		_rtl92de_enable_bcn_sub_func(hw);
	} else if (type == NL80211_IFTYPE_ADHOC ||
		type == NL80211_IFTYPE_AP) {
		_rtl92de_resume_tx_beacon(hw);
		_rtl92de_disable_bcn_sub_func(hw);
	} else {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
			 "Set HW_VAR_MEDIA_STATUS: No such media status(%x)\n",
			 type);
	}
	bcnfunc_enable = rtl_read_byte(rtlpriv, REG_BCN_CTRL);
	switch (type) {
	case NL80211_IFTYPE_UNSPECIFIED:
		bt_msr |= MSR_NOLINK;
		ledaction = LED_CTL_LINK;
		bcnfunc_enable &= 0xF7;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to NO LINK!\n");
		break;
	case NL80211_IFTYPE_ADHOC:
		bt_msr |= MSR_ADHOC;
		bcnfunc_enable |= 0x08;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to Ad Hoc!\n");
		break;
	case NL80211_IFTYPE_STATION:
		bt_msr |= MSR_INFRA;
		ledaction = LED_CTL_LINK;
		bcnfunc_enable &= 0xF7;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to STA!\n");
		break;
	case NL80211_IFTYPE_AP:
		bt_msr |= MSR_AP;
		bcnfunc_enable |= 0x08;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to AP!\n");
		break;
	default:
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Network type %d not supported!\n", type);
		return 1;
		break;

	}
	rtl_write_byte(rtlpriv, MSR, bt_msr);
	rtlpriv->cfg->ops->led_control(hw, ledaction);
	if ((bt_msr & MSR_MASK) == MSR_AP)
		rtl_write_byte(rtlpriv, REG_BCNTCFG + 1, 0x00);
	else
		rtl_write_byte(rtlpriv, REG_BCNTCFG + 1, 0x66);
	return 0;
}
