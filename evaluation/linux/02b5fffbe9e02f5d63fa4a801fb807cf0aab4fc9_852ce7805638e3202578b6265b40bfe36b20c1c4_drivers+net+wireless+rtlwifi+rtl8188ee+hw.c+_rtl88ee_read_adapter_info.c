static void _rtl88ee_read_adapter_info(struct ieee80211_hw *hw)
{
	struct rtl_priv *rtlpriv = rtl_priv(hw);
	struct rtl_efuse *rtlefuse = rtl_efuse(rtl_priv(hw));
	struct rtl_hal *rtlhal = rtl_hal(rtl_priv(hw));
	u16 i, usvalue;
	u8 hwinfo[HWSET_MAX_SIZE];
	u16 eeprom_id;

	if (rtlefuse->epromtype == EEPROM_BOOT_EFUSE) {
		rtl_efuse_shadow_map_update(hw);

		memcpy(hwinfo, &rtlefuse->efuse_map[EFUSE_INIT_MAP][0],
		       HWSET_MAX_SIZE);
	} else if (rtlefuse->epromtype == EEPROM_93C46) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "RTL819X Not boot from eeprom, check it !!");
		return;
	} else {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_EMERG,
			 "boot from neither eeprom nor efuse, check it !!");
		return;
	}

	RT_PRINT_DATA(rtlpriv, COMP_INIT, DBG_DMESG, "MAP\n",
		      hwinfo, HWSET_MAX_SIZE);

	eeprom_id = *((u16 *)&hwinfo[0]);
	if (eeprom_id != RTL8188E_EEPROM_ID) {
		RT_TRACE(rtlpriv, COMP_ERR, DBG_WARNING,
			 "EEPROM ID(%#x) is invalid!!\n", eeprom_id);
		rtlefuse->autoload_failflag = true;
	} else {
		RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD, "Autoload OK\n");
		rtlefuse->autoload_failflag = false;
	}

	if (rtlefuse->autoload_failflag == true)
		return;
	/*VID DID SVID SDID*/
	rtlefuse->eeprom_vid = *(u16 *)&hwinfo[EEPROM_VID];
	rtlefuse->eeprom_did = *(u16 *)&hwinfo[EEPROM_DID];
	rtlefuse->eeprom_svid = *(u16 *)&hwinfo[EEPROM_SVID];
	rtlefuse->eeprom_smid = *(u16 *)&hwinfo[EEPROM_SMID];
	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "EEPROMId = 0x%4x\n", eeprom_id);
	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "EEPROM VID = 0x%4x\n", rtlefuse->eeprom_vid);
	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "EEPROM DID = 0x%4x\n", rtlefuse->eeprom_did);
	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "EEPROM SVID = 0x%4x\n", rtlefuse->eeprom_svid);
	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "EEPROM SMID = 0x%4x\n", rtlefuse->eeprom_smid);
	/*customer ID*/
	rtlefuse->eeprom_oemid = hwinfo[EEPROM_CUSTOMER_ID];
	if (rtlefuse->eeprom_oemid == 0xFF)
		rtlefuse->eeprom_oemid =	0;

	RT_TRACE(rtlpriv, COMP_INIT, DBG_LOUD,
		 "EEPROM Customer ID: 0x%2x\n", rtlefuse->eeprom_oemid);
	/*EEPROM version*/
	rtlefuse->eeprom_version = *(u16 *)&hwinfo[EEPROM_VERSION];
	/*mac address*/
	for (i = 0; i < 6; i += 2) {
		usvalue = *(u16 *)&hwinfo[EEPROM_MAC_ADDR + i];
		*((u16 *)(&rtlefuse->dev_addr[i])) = usvalue;
	}

	RT_TRACE(rtlpriv, COMP_INIT, DBG_DMESG,
		 "dev_addr: %pM\n", rtlefuse->dev_addr);
	/*channel plan */
	rtlefuse->eeprom_channelplan = hwinfo[EEPROM_CHANNELPLAN];
	/* set channel plan from efuse */
	rtlefuse->channel_plan = rtlefuse->eeprom_channelplan;
	/*tx power*/
	_rtl88ee_read_txpower_info_from_hwpg(hw,
					     rtlefuse->autoload_failflag,
					     hwinfo);
	rtlefuse->txpwr_fromeprom = true;

	rtl8188ee_read_bt_coexist_info_from_hwpg(hw,
						 rtlefuse->autoload_failflag,
						 hwinfo);

	/*board type*/
	rtlefuse->board_type =
		((hwinfo[EEPROM_RF_BOARD_OPTION_88E] & 0xE0) >> 5);
	rtlhal->board_type = rtlefuse->board_type;
	/*Wake on wlan*/
	rtlefuse->wowlan_enable =
		((hwinfo[EEPROM_RF_FEATURE_OPTION_88E] & 0x40) >> 6);
	/*parse xtal*/
	rtlefuse->crystalcap = hwinfo[EEPROM_XTAL_88E];
	if (hwinfo[EEPROM_XTAL_88E])
		rtlefuse->crystalcap = 0x20;
	/*antenna diversity*/
	rtlefuse->antenna_div_cfg =
		(hwinfo[EEPROM_RF_BOARD_OPTION_88E] & 0x18) >> 3;
	if (hwinfo[EEPROM_RF_BOARD_OPTION_88E] == 0xFF)
		rtlefuse->antenna_div_cfg = 0;
	if (rtlpriv->btcoexist.eeprom_bt_coexist != 0 &&
	    rtlpriv->btcoexist.eeprom_bt_ant_num == ANT_X1)
		rtlefuse->antenna_div_cfg = 0;

	rtlefuse->antenna_div_type = hwinfo[EEPROM_RF_ANTENNA_OPT_88E];
	if (rtlefuse->antenna_div_type == 0xFF)
		rtlefuse->antenna_div_type = 0x01;
	if (rtlefuse->antenna_div_type == CG_TRX_HW_ANTDIV ||
		rtlefuse->antenna_div_type == CGCS_RX_HW_ANTDIV)
		rtlefuse->antenna_div_cfg = 1;

	if (rtlhal->oem_id == RT_CID_DEFAULT) {
		switch (rtlefuse->eeprom_oemid) {
		case EEPROM_CID_DEFAULT:
			if (rtlefuse->eeprom_did == 0x8179) {
				if (rtlefuse->eeprom_svid == 0x1025) {
					rtlhal->oem_id = RT_CID_819X_ACER;
				} else if ((rtlefuse->eeprom_svid == 0x10EC &&
				     rtlefuse->eeprom_smid == 0x0179) ||
				     (rtlefuse->eeprom_svid == 0x17AA &&
				     rtlefuse->eeprom_smid == 0x0179)) {
					rtlhal->oem_id = RT_CID_819X_LENOVO;
				} else if (rtlefuse->eeprom_svid == 0x103c &&
					   rtlefuse->eeprom_smid == 0x197d) {
					rtlhal->oem_id = RT_CID_819X_HP;
				} else {
					rtlhal->oem_id = RT_CID_DEFAULT;
				}
			} else {
				rtlhal->oem_id = RT_CID_DEFAULT;
			}
			break;
		case EEPROM_CID_TOSHIBA:
			rtlhal->oem_id = RT_CID_TOSHIBA;
			break;
		case EEPROM_CID_QMI:
			rtlhal->oem_id = RT_CID_819X_QMI;
			break;
		case EEPROM_CID_WHQL:
		default:
			rtlhal->oem_id = RT_CID_DEFAULT;
			break;

		}
	}
}
