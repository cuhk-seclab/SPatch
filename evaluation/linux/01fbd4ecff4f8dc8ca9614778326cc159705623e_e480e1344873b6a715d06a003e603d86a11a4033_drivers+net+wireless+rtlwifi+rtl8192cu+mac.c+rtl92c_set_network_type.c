int rtl92c_set_network_type(struct ieee80211_hw *hw, enum nl80211_iftype type)
{
	u8 value;
	struct rtl_priv *rtlpriv = rtl_priv(hw);

	switch (type) {
	case NL80211_IFTYPE_UNSPECIFIED:
		value = NT_NO_LINK;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG,
			 "Set Network type to NO LINK!\n");
		break;
	case NL80211_IFTYPE_ADHOC:
		value = NT_LINK_AD_HOC;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG,
			 "Set Network type to Ad Hoc!\n");
		break;
	case NL80211_IFTYPE_STATION:
		value = NT_LINK_AP;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG,
			 "Set Network type to STA!\n");
		break;
	case NL80211_IFTYPE_AP:
		value = NT_AS_AP;
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG,
			 "Set Network type to AP!\n");
		break;
	default:
		RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG,
			 "Network type %d not supported!\n", type);
		return -EOPNOTSUPP;
	}
	rtl_write_byte(rtlpriv, MSR, value);
	return 0;
}
