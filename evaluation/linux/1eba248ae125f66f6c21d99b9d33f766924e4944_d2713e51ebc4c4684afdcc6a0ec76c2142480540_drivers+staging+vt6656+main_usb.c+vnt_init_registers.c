static int vnt_init_registers(struct vnt_private *priv)
{
	struct vnt_cmd_card_init *init_cmd = &priv->init_command;
	struct vnt_rsp_card_init *init_rsp = &priv->init_response;
	u8 antenna;
	int ii;
	int status = STATUS_SUCCESS;
	u8 tmp;
	u8 calib_tx_iq = 0, calib_tx_dc = 0, calib_rx_iq = 0;

	dev_dbg(&priv->usb->dev, "---->INIbInitAdapter. [%d][%d]\n",
				DEVICE_INIT_COLD, priv->packet_type);

	if (!vnt_check_firmware_version(priv)) {
		if (vnt_download_firmware(priv) == true) {
			if (vnt_firmware_branch_to_sram(priv) == false) {
				dev_dbg(&priv->usb->dev,
					" vnt_firmware_branch_to_sram fail\n");
				return false;
			}
		} else {
			dev_dbg(&priv->usb->dev, "FIRMWAREbDownload fail\n");
			return false;
		}
	}

	if (!vnt_vt3184_init(priv)) {
		dev_dbg(&priv->usb->dev, "vnt_vt3184_init fail\n");
		return false;
	}

	init_cmd->init_class = DEVICE_INIT_COLD;
	init_cmd->exist_sw_net_addr = priv->exist_sw_net_addr;
	for (ii = 0; ii < 6; ii++)
		init_cmd->sw_net_addr[ii] = priv->current_net_addr[ii];
	init_cmd->short_retry_limit = priv->short_retry_limit;
	init_cmd->long_retry_limit = priv->long_retry_limit;

	/* issue card_init command to device */
	status = vnt_control_out(priv,
		MESSAGE_TYPE_CARDINIT, 0, 0,
		sizeof(struct vnt_cmd_card_init), (u8 *)init_cmd);
	if (status != STATUS_SUCCESS) {
		dev_dbg(&priv->usb->dev, "Issue Card init fail\n");
		return false;
	}

	status = vnt_control_in(priv, MESSAGE_TYPE_INIT_RSP, 0, 0,
		sizeof(struct vnt_rsp_card_init), (u8 *)init_rsp);
	if (status != STATUS_SUCCESS) {
		dev_dbg(&priv->usb->dev,
			"Cardinit request in status fail!\n");
		return false;
	}

	/* local ID for AES functions */
	status = vnt_control_in(priv, MESSAGE_TYPE_READ,
		MAC_REG_LOCALID, MESSAGE_REQUEST_MACREG, 1,
			&priv->local_id);
	if (status != STATUS_SUCCESS)
		return false;

	/* do MACbSoftwareReset in MACvInitialize */

	priv->top_ofdm_basic_rate = RATE_24M;
	priv->top_cck_basic_rate = RATE_1M;

	/* target to IF pin while programming to RF chip */
	priv->power = 0xFF;

	priv->cck_pwr = priv->eeprom[EEP_OFS_PWR_CCK];
	priv->ofdm_pwr_g = priv->eeprom[EEP_OFS_PWR_OFDMG];
	/* load power table */
	for (ii = 0; ii < 14; ii++) {
		priv->cck_pwr_tbl[ii] =
			priv->eeprom[ii + EEP_OFS_CCK_PWR_TBL];
		if (priv->cck_pwr_tbl[ii] == 0)
			priv->cck_pwr_tbl[ii] = priv->cck_pwr;

		priv->ofdm_pwr_tbl[ii] =
				priv->eeprom[ii + EEP_OFS_OFDM_PWR_TBL];
		if (priv->ofdm_pwr_tbl[ii] == 0)
			priv->ofdm_pwr_tbl[ii] = priv->ofdm_pwr_g;
	}

	/*
	 * original zonetype is USA, but custom zonetype is Europe,
	 * then need to recover 12, 13, 14 channels with 11 channel
	 */
	for (ii = 11; ii < 14; ii++) {
		priv->cck_pwr_tbl[ii] = priv->cck_pwr_tbl[10];
		priv->ofdm_pwr_tbl[ii] = priv->ofdm_pwr_tbl[10];
	}

	priv->ofdm_pwr_a = 0x34; /* same as RFbMA2829SelectChannel */

	/* load OFDM A power table */
	for (ii = 0; ii < CB_MAX_CHANNEL_5G; ii++) {
		priv->ofdm_a_pwr_tbl[ii] =
			priv->eeprom[ii + EEP_OFS_OFDMA_PWR_TBL];

		if (priv->ofdm_a_pwr_tbl[ii] == 0)
			priv->ofdm_a_pwr_tbl[ii] = priv->ofdm_pwr_a;
	}

	antenna = priv->eeprom[EEP_OFS_ANTENNA];

	if (antenna & EEP_ANTINV)
		priv->tx_rx_ant_inv = true;
	else
		priv->tx_rx_ant_inv = false;

	antenna &= (EEP_ANTENNA_AUX | EEP_ANTENNA_MAIN);

	if (antenna == 0) /* if not set default is both */
		antenna = (EEP_ANTENNA_AUX | EEP_ANTENNA_MAIN);

	if (antenna == (EEP_ANTENNA_AUX | EEP_ANTENNA_MAIN)) {
		priv->tx_antenna_mode = ANT_B;
		priv->rx_antenna_sel = 1;

		if (priv->tx_rx_ant_inv == true)
			priv->rx_antenna_mode = ANT_A;
		else
			priv->rx_antenna_mode = ANT_B;
	} else  {
		priv->rx_antenna_sel = 0;

		if (antenna & EEP_ANTENNA_AUX) {
			priv->tx_antenna_mode = ANT_A;

			if (priv->tx_rx_ant_inv == true)
				priv->rx_antenna_mode = ANT_B;
			else
				priv->rx_antenna_mode = ANT_A;
		} else {
			priv->tx_antenna_mode = ANT_B;

		if (priv->tx_rx_ant_inv == true)
			priv->rx_antenna_mode = ANT_A;
		else
			priv->rx_antenna_mode = ANT_B;
		}
	}

	/* Set initial antenna mode */
	vnt_set_antenna_mode(priv, priv->rx_antenna_mode);

	/* get Auto Fall Back type */
	priv->auto_fb_ctrl = AUTO_FB_0;

	/* default Auto Mode */
	priv->bb_type = BB_TYPE_11G;

	/* get RFType */
	priv->rf_type = init_rsp->rf_type;

	/* load vt3266 calibration parameters in EEPROM */
	if (priv->rf_type == RF_VT3226D0) {
		if ((priv->eeprom[EEP_OFS_MAJOR_VER] == 0x1) &&
		    (priv->eeprom[EEP_OFS_MINOR_VER] >= 0x4)) {

			calib_tx_iq = priv->eeprom[EEP_OFS_CALIB_TX_IQ];
			calib_tx_dc = priv->eeprom[EEP_OFS_CALIB_TX_DC];
			calib_rx_iq = priv->eeprom[EEP_OFS_CALIB_RX_IQ];
			if (calib_tx_iq || calib_tx_dc || calib_rx_iq) {
				/* CR255, enable TX/RX IQ and
				 * DC compensation mode
				 */
				vnt_control_out_u8(priv,
						   MESSAGE_REQUEST_BBREG,
						   0xff,
						   0x03);
				/* CR251, TX I/Q Imbalance Calibration */
				vnt_control_out_u8(priv,
						   MESSAGE_REQUEST_BBREG,
						   0xfb,
						   calib_tx_iq);
				/* CR252, TX DC-Offset Calibration */
				vnt_control_out_u8(priv,
						   MESSAGE_REQUEST_BBREG,
						   0xfC,
						   calib_tx_dc);
				/* CR253, RX I/Q Imbalance Calibration */
				vnt_control_out_u8(priv,
						   MESSAGE_REQUEST_BBREG,
						   0xfd,
						   calib_rx_iq);
			} else {
				/* CR255, turn off
				 * BB Calibration compensation
				 */
				vnt_control_out_u8(priv,
						   MESSAGE_REQUEST_BBREG,
						   0xff,
						   0x0);
			}
		}
	}

	/* get permanent network address */
	memcpy(priv->permanent_net_addr, init_rsp->net_addr, 6);
	ether_addr_copy(priv->current_net_addr, priv->permanent_net_addr);

	/* if exist SW network address, use it */
	dev_dbg(&priv->usb->dev, "Network address = %pM\n",
		priv->current_net_addr);

	/*
	* set BB and packet type at the same time
	* set Short Slot Time, xIFS, and RSPINF
	*/
	if (priv->bb_type == BB_TYPE_11A)
		priv->short_slot_time = true;
	else
		priv->short_slot_time = false;

	vnt_set_short_slot_time(priv);

	priv->radio_ctl = priv->eeprom[EEP_OFS_RADIOCTL];

	if ((priv->radio_ctl & EEP_RADIOCTL_ENABLE) != 0) {
		status = vnt_control_in(priv, MESSAGE_TYPE_READ,
			MAC_REG_GPIOCTL1, MESSAGE_REQUEST_MACREG, 1, &tmp);

		if (status != STATUS_SUCCESS)
			return false;

		if ((tmp & GPIO3_DATA) == 0)
			vnt_mac_reg_bits_on(priv, MAC_REG_GPIOCTL1,
								GPIO3_INTMD);
		else
			vnt_mac_reg_bits_off(priv, MAC_REG_GPIOCTL1,
								GPIO3_INTMD);
	}

	vnt_mac_set_led(priv, LEDSTS_TMLEN, 0x38);

	vnt_mac_set_led(priv, LEDSTS_STS, LEDSTS_SLOW);

	vnt_mac_reg_bits_on(priv, MAC_REG_GPIOCTL0, 0x01);

	vnt_radio_power_on(priv);

	dev_dbg(&priv->usb->dev, "<----INIbInitAdapter Exit\n");

	return true;
}
