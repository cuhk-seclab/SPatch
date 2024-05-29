bool rtl8192_SetRFPowerState(struct net_device *dev,
			     RT_RF_POWER_STATE eRFPowerState)
{
	bool				bResult = true;
	struct r8192_priv *priv = ieee80211_priv(dev);

	if (eRFPowerState == priv->ieee80211->eRFPowerState)
		return false;

	if (priv->SetRFPowerStateInProgress)
		return false;

	priv->SetRFPowerStateInProgress = true;

	switch (priv->rf_chip) {
	case RF_8256:
		switch (eRFPowerState) {
		case eRfOn:
			/* RF-A, RF-B */
			/* enable RF-Chip A/B - 0x860[4] */
			rtl8192_setBBreg(dev, rFPGA0_XA_RFInterfaceOE, BIT4,
					 0x1);
			/* analog to digital on - 0x88c[9:8] */
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0x300,
					 0x3);
			/* digital to analog on - 0x880[4:3] */
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, 0x18,
					 0x3);
			/* rx antenna on - 0xc04[1:0] */
			rtl8192_setBBreg(dev, rOFDM0_TRxPathEnable, 0x3, 0x3);
			/* rx antenna on - 0xd04[1:0] */
			rtl8192_setBBreg(dev, rOFDM1_TRxPathEnable, 0x3, 0x3);
			/* analog to digital part2 on - 0x880[6:5] */
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, 0x60,
					 0x3);

			break;

		case eRfSleep:

			break;

		case eRfOff:
			/* RF-A, RF-B */
			/* disable RF-Chip A/B - 0x860[4] */
			rtl8192_setBBreg(dev, rFPGA0_XA_RFInterfaceOE, BIT4,
					 0x0);
			/* analog to digital off, for power save */
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter4, 0xf00,
					 0x0); /* 0x88c[11:8] */
			/* digital to analog off, for power save - 0x880[4:3] */
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, 0x18,
					 0x0);
			/* rx antenna off - 0xc04[3:0] */
			rtl8192_setBBreg(dev, rOFDM0_TRxPathEnable, 0xf, 0x0);
			/* rx antenna off - 0xd04[3:0] */
			rtl8192_setBBreg(dev, rOFDM1_TRxPathEnable, 0xf, 0x0);
			/* analog to digital part2 off, for power save */
			rtl8192_setBBreg(dev, rFPGA0_AnalogParameter1, 0x60,
					 0x0); /* 0x880[6:5] */

			break;

		default:
			bResult = false;
			RT_TRACE(COMP_ERR, "%s(): unknown state to set: 0x%X\n",
				 __func__, eRFPowerState);
			break;
		}
		break;
	default:
		RT_TRACE(COMP_ERR, "Not support rf_chip(%x)\n", priv->rf_chip);
		break;
	}
#ifdef TO_DO_LIST
	if (bResult) {
		/* Update current RF state variable. */
		pHalData->eRFPowerState = eRFPowerState;
		switch (pHalData->RFChipID) {
		case RF_8256:
			switch (pHalData->eRFPowerState) {
			case eRfOff:
				/* If Rf off reason is from IPS,
				   LED should blink with no link */
				if (pMgntInfo->RfOffReason == RF_CHANGE_BY_IPS)
					Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_NO_LINK);
				else
					/* Turn off LED if RF is not ON. */
					Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_POWER_OFF);
				break;

			case eRfOn:
				/* Turn on RF we are still linked, which might
				   happen when we quickly turn off and on HW RF.
				 */
				if (pMgntInfo->bMediaConnect)
					Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_LINK);
				else
					/* Turn off LED if RF is not ON. */
					Adapter->HalFunc.LedControlHandler(Adapter, LED_CTL_NO_LINK);
				break;

			default:
				break;
			}
			break;

		default:
			RT_TRACE(COMP_RF, DBG_LOUD, "%s(): Unknown RF type\n",
				 __func__);
			break;
		}

	}
#endif
	priv->SetRFPowerStateInProgress = false;

	return bResult;
}
