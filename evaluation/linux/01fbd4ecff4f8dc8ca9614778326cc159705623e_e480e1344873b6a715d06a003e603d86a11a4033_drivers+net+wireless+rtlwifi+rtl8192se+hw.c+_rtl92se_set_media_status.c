static int _rtl92se_set_media_status(struct ieee80211_hw *hw,
				     enum nl80211_iftype type)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	u8 bt_msr = rtl_read_byte(rtlpriv, MSR);
	u32 temp;
	bt_msr &= ~MSR_LINK_MASK;

	switch (type) {
	case NL80211_IFTYPE_UNSPECIFIED:
		bt_msr |= (MSR_LINK_NONE << MSR_LINK_SHIFT);
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to NO LINK!\n");
		break;
	case NL80211_IFTYPE_ADHOC:
		bt_msr |= (MSR_LINK_ADHOC << MSR_LINK_SHIFT);
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to Ad Hoc!\n");
		break;
	case NL80211_IFTYPE_STATION:
		bt_msr |= (MSR_LINK_MANAGED << MSR_LINK_SHIFT);
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to STA!\n");
		break;
	case NL80211_IFTYPE_AP:
		bt_msr |= (MSR_LINK_MASTER << MSR_LINK_SHIFT);
		RT_TRACE(rtlpriv, COMP_INIT, DBG_TRACE,
			 "Set Network type to AP!\n");
		break;
	default:
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "Network type %d not supported!\n", type);
		return 1;

	}

	if (type != NL80211_IFTYPE_AP &&
	    rtlpriv->mac80211.link_state < MAC80211_LINKED)
		bt_msr = rtl_read_byte(rtlpriv, MSR) & ~MSR_LINK_MASK;
	rtl_write_byte(rtlpriv, MSR, bt_msr);

	temp = rtl_read_dword(rtlpriv, TCR);
	rtl_write_dword(rtlpriv, TCR, temp & (~BIT(8)));
	rtl_write_dword(rtlpriv, TCR, temp | BIT(8));


	return 0;
}
