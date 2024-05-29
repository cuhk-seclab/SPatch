static bool rtl8192_adapter_start(struct net_device *dev)
{
	struct r8192_priv *priv = ieee80211_priv(dev);
	u32 dwRegRead = 0;
	bool init_status = true;
	u8 SECR_value = 0x0;
	u8 tmp;
	RT_TRACE(COMP_INIT, "====>%s()\n", __func__);
	priv->Rf_Mode = RF_OP_By_SW_3wire;
	//for ASIC power on sequence
	write_nic_byte_E(dev, 0x5f, 0x80);
	mdelay(50);
	write_nic_byte_E(dev, 0x5f, 0xf0);
	write_nic_byte_E(dev, 0x5d, 0x00);
	write_nic_byte_E(dev, 0x5e, 0x80);
	write_nic_byte(dev, 0x17, 0x37);
	mdelay(10);
	priv->pFirmware->firmware_status = FW_STATUS_0_INIT;
	//config CPUReset Register
	//Firmware Reset or not?
	read_nic_dword(dev, CPU_GEN, &dwRegRead);
	if (priv->pFirmware->firmware_status == FW_STATUS_0_INIT)
		dwRegRead |= CPU_GEN_SYSTEM_RESET; //do nothing here?
	else if (priv->pFirmware->firmware_status == FW_STATUS_5_READY)
		dwRegRead |= CPU_GEN_FIRMWARE_RESET;
	else
		RT_TRACE(COMP_ERR, "ERROR in %s(): undefined firmware state(%d)\n", __func__,   priv->pFirmware->firmware_status);

	write_nic_dword(dev, CPU_GEN, dwRegRead);
	//config BB.
	rtl8192_BBConfig(dev);

	//Loopback mode or not
	priv->LoopbackMode = RTL819xU_NO_LOOPBACK;

	read_nic_dword(dev, CPU_GEN, &dwRegRead);
	if (priv->LoopbackMode == RTL819xU_NO_LOOPBACK)
		dwRegRead = ((dwRegRead & CPU_GEN_NO_LOOPBACK_MSK) | CPU_GEN_NO_LOOPBACK_SET);
	else if (priv->LoopbackMode == RTL819xU_MAC_LOOPBACK)
		dwRegRead |= CPU_CCK_LOOPBACK;
	else
		RT_TRACE(COMP_ERR, "Serious error in %s(): wrong loopback mode setting(%d)\n", __func__,  priv->LoopbackMode);

	write_nic_dword(dev, CPU_GEN, dwRegRead);

	//after reset cpu, we need wait for a seconds to write in register.
	udelay(500);

	//xiong add for new bitfile:usb suspend reset pin set to 1. //do we need?
	read_nic_byte_E(dev, 0x5f, &tmp);
	write_nic_byte_E(dev, 0x5f, tmp|0x20);

	//Set Hardware
	rtl8192_hwconfig(dev);

	//turn on Tx/Rx
	write_nic_byte(dev, CMDR, CR_RE|CR_TE);

	//set IDR0 here
	write_nic_dword(dev, MAC0, ((u32 *)dev->dev_addr)[0]);
	write_nic_word(dev, MAC4, ((u16 *)(dev->dev_addr + 4))[0]);

	//set RCR
	write_nic_dword(dev, RCR, priv->ReceiveConfig);

	//Initialize Number of Reserved Pages in Firmware Queue
	write_nic_dword(dev, RQPN1,  NUM_OF_PAGE_IN_FW_QUEUE_BK << RSVD_FW_QUEUE_PAGE_BK_SHIFT |
			NUM_OF_PAGE_IN_FW_QUEUE_BE << RSVD_FW_QUEUE_PAGE_BE_SHIFT |
			NUM_OF_PAGE_IN_FW_QUEUE_VI << RSVD_FW_QUEUE_PAGE_VI_SHIFT |
			NUM_OF_PAGE_IN_FW_QUEUE_VO <<RSVD_FW_QUEUE_PAGE_VO_SHIFT);
	write_nic_dword(dev, RQPN2, NUM_OF_PAGE_IN_FW_QUEUE_MGNT << RSVD_FW_QUEUE_PAGE_MGNT_SHIFT |
			NUM_OF_PAGE_IN_FW_QUEUE_CMD << RSVD_FW_QUEUE_PAGE_CMD_SHIFT);
	write_nic_dword(dev, RQPN3, APPLIED_RESERVED_QUEUE_IN_FW|
			NUM_OF_PAGE_IN_FW_QUEUE_BCN<<RSVD_FW_QUEUE_PAGE_BCN_SHIFT);
	write_nic_dword(dev, RATR0+4*7, (RATE_ALL_OFDM_AG | RATE_ALL_CCK));

	//Set AckTimeout
	// TODO: (it value is only for FPGA version). need to be changed!!2006.12.18, by Emily
	write_nic_byte(dev, ACK_TIMEOUT, 0x30);

	if (priv->ResetProgress == RESET_TYPE_NORESET)
		rtl8192_SetWirelessMode(dev, priv->ieee80211->mode);
	if (priv->ResetProgress == RESET_TYPE_NORESET) {
		CamResetAllEntry(dev);
		SECR_value |= SCR_TxEncEnable;
		SECR_value |= SCR_RxDecEnable;
		SECR_value |= SCR_NoSKMC;
		write_nic_byte(dev, SECR, SECR_value);
	}

	//Beacon related
	write_nic_word(dev, ATIMWND, 2);
	write_nic_word(dev, BCN_INTERVAL, 100);

#define DEFAULT_EDCA 0x005e4332
	{
		int i;
		for (i = 0; i < QOS_QUEUE_NUM; i++)
			write_nic_dword(dev, WDCAPARA_ADD[i], DEFAULT_EDCA);
	}

	rtl8192_phy_configmac(dev);

	if (priv->card_8192_version == (u8) VERSION_819xU_A) {
		rtl8192_phy_getTxPower(dev);
		rtl8192_phy_setTxPower(dev, priv->chan);
	}

	//Firmware download
	init_status = init_firmware(dev);
	if (!init_status) {
		RT_TRACE(COMP_ERR, "ERR!!! %s(): Firmware download is failed\n", __func__);
		return init_status;
	}
	RT_TRACE(COMP_INIT, "%s():after firmware download\n", __func__);
	//
#ifdef TO_DO_LIST
	if (Adapter->ResetProgress == RESET_TYPE_NORESET) {
		if (pMgntInfo->RegRfOff == true) { /* User disable RF via registry. */
			RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter819xUsb(): Turn off RF for RegRfOff ----------\n"));
			MgntActSet_RF_State(Adapter, eRfOff, RF_CHANGE_BY_SW);
			// Those actions will be discard in MgntActSet_RF_State because of the same state
			for (eRFPath = 0; eRFPath < pHalData->NumTotalRFPath; eRFPath++)
				PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x4, 0xC00, 0x0);
		} else if (pMgntInfo->RfOffReason > RF_CHANGE_BY_PS) { /* H/W or S/W RF OFF before sleep. */
			RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter819xUsb(): Turn off RF for RfOffReason(%d) ----------\n", pMgntInfo->RfOffReason));
			MgntActSet_RF_State(Adapter, eRfOff, pMgntInfo->RfOffReason);
		} else {
			pHalData->eRFPowerState = eRfOn;
			pMgntInfo->RfOffReason = 0;
			RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter819xUsb(): RF is on ----------\n"));
		}
	} else {
		if (pHalData->eRFPowerState == eRfOff) {
			MgntActSet_RF_State(Adapter, eRfOff, pMgntInfo->RfOffReason);
			// Those actions will be discard in MgntActSet_RF_State because of the same state
			for (eRFPath = 0; eRFPath < pHalData->NumTotalRFPath; eRFPath++)
				PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x4, 0xC00, 0x0);
		}
	}
#endif
	//config RF.
	if (priv->ResetProgress == RESET_TYPE_NORESET) {
		rtl8192_phy_RFConfig(dev);
		RT_TRACE(COMP_INIT, "%s():after phy RF config\n", __func__);
	}


	if (priv->ieee80211->FwRWRF)
		// We can force firmware to do RF-R/W
		priv->Rf_Mode = RF_OP_By_FW;
	else
		priv->Rf_Mode = RF_OP_By_SW_3wire;


	rtl8192_phy_updateInitGain(dev);
	/*--set CCK and OFDM Block "ON"--*/
	rtl8192_setBBreg(dev, rFPGA0_RFMOD, bCCKEn, 0x1);
	rtl8192_setBBreg(dev, rFPGA0_RFMOD, bOFDMEn, 0x1);

	if (priv->ResetProgress == RESET_TYPE_NORESET) {
		//if D or C cut
		u8 tmpvalue;
		read_nic_byte(dev, 0x301, &tmpvalue);
		if (tmpvalue == 0x03) {
			priv->bDcut = true;
			RT_TRACE(COMP_POWER_TRACKING, "D-cut\n");
		} else {
			priv->bDcut = false;
			RT_TRACE(COMP_POWER_TRACKING, "C-cut\n");
		}
		dm_initialize_txpower_tracking(dev);

		if (priv->bDcut) {
			u32 i, TempCCk;
			u32 tmpRegA = rtl8192_QueryBBReg(dev, rOFDM0_XATxIQImbalance, bMaskDWord);
			for (i = 0; i < TxBBGainTableLength; i++) {
				if (tmpRegA == priv->txbbgain_table[i].txbbgain_value) {
					priv->rfa_txpowertrackingindex = (u8)i;
					priv->rfa_txpowertrackingindex_real = (u8)i;
					priv->rfa_txpowertracking_default = priv->rfa_txpowertrackingindex;
					break;
				}
			}

			TempCCk = rtl8192_QueryBBReg(dev, rCCK0_TxFilter1, bMaskByte2);

			for (i = 0; i < CCKTxBBGainTableLength; i++) {

				if (TempCCk == priv->cck_txbbgain_table[i].ccktxbb_valuearray[0]) {
					priv->cck_present_attentuation_20Mdefault = (u8) i;
					break;
				}
			}
			priv->cck_present_attentuation_40Mdefault = 0;
			priv->cck_present_attentuation_difference = 0;
			priv->cck_present_attentuation = priv->cck_present_attentuation_20Mdefault;

		}
	}
	write_nic_byte(dev, 0x87, 0x0);


	return init_status;
}
