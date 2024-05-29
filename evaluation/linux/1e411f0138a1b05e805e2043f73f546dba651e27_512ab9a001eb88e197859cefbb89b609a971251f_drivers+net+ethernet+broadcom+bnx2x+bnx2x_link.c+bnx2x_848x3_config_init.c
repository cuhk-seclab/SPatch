static int bnx2x_848x3_config_init(struct bnx2x_phy *phy,
				   struct link_params *params,
				   struct link_vars *vars)
{
	struct bnx2x *bp = params->bp;
	u8 port, initialize = 1;
	u16 val;
	u32 actual_phy_selection;
	u16 cmd_args[PHY848xx_CMDHDLR_MAX_ARGS];
	int rc = 0;

	usleep_range(1000, 2000);

	if (!(CHIP_IS_E1x(bp)))
		port = BP_PATH(bp);
	else
		port = params->port;

	if (phy->type == PORT_HW_CFG_XGXS_EXT_PHY_TYPE_BCM84823) {
		bnx2x_set_gpio(bp, MISC_REGISTERS_GPIO_3,
			       MISC_REGISTERS_GPIO_OUTPUT_HIGH,
			       port);
	} else {
		/* MDIO reset */
		bnx2x_cl45_write(bp, phy,
				MDIO_PMA_DEVAD,
				MDIO_PMA_REG_CTRL, 0x8000);
	}

	bnx2x_wait_reset_complete(bp, phy, params);

	/* Wait for GPHY to come out of reset */
	msleep(50);
	if (!bnx2x_is_8483x_8485x(phy)) {
		/* BCM84823 requires that XGXS links up first @ 10G for normal
		 * behavior.
		 */
		u16 temp;
		temp = vars->line_speed;
		vars->line_speed = SPEED_10000;
		bnx2x_set_autoneg(&params->phy[INT_PHY], params, vars, 0);
		bnx2x_program_serdes(&params->phy[INT_PHY], params, vars);
		vars->line_speed = temp;
	}
	/* Check if this is actually BCM84858 */
	if (phy->type != PORT_HW_CFG_XGXS_EXT_PHY_TYPE_BCM84858) {
		u16 hw_rev;

		bnx2x_cl45_read(bp, phy, MDIO_AN_DEVAD,
				MDIO_AN_REG_848xx_ID_MSB, &hw_rev);
		if (hw_rev == BCM84858_PHY_ID) {
			params->link_attr_sync |= LINK_ATTR_84858;
			bnx2x_update_link_attr(params, params->link_attr_sync);
		}
	}

	/* Set dual-media configuration according to configuration */
	bnx2x_cl45_read(bp, phy, MDIO_CTL_DEVAD,
			MDIO_CTL_REG_84823_MEDIA, &val);
	val &= ~(MDIO_CTL_REG_84823_MEDIA_MAC_MASK |
		 MDIO_CTL_REG_84823_MEDIA_LINE_MASK |
		 MDIO_CTL_REG_84823_MEDIA_COPPER_CORE_DOWN |
		 MDIO_CTL_REG_84823_MEDIA_PRIORITY_MASK |
		 MDIO_CTL_REG_84823_MEDIA_FIBER_1G);

	if (CHIP_IS_E3(bp)) {
		val &= ~(MDIO_CTL_REG_84823_MEDIA_MAC_MASK |
			 MDIO_CTL_REG_84823_MEDIA_LINE_MASK);
	} else {
		val |= (MDIO_CTL_REG_84823_CTRL_MAC_XFI |
			MDIO_CTL_REG_84823_MEDIA_LINE_XAUI_L);
	}

	actual_phy_selection = bnx2x_phy_selection(params);

	switch (actual_phy_selection) {
	case PORT_HW_CFG_PHY_SELECTION_HARDWARE_DEFAULT:
		/* Do nothing. Essentially this is like the priority copper */
		break;
	case PORT_HW_CFG_PHY_SELECTION_FIRST_PHY_PRIORITY:
		val |= MDIO_CTL_REG_84823_MEDIA_PRIORITY_COPPER;
		break;
	case PORT_HW_CFG_PHY_SELECTION_SECOND_PHY_PRIORITY:
		val |= MDIO_CTL_REG_84823_MEDIA_PRIORITY_FIBER;
		break;
	case PORT_HW_CFG_PHY_SELECTION_FIRST_PHY:
		/* Do nothing here. The first PHY won't be initialized at all */
		break;
	case PORT_HW_CFG_PHY_SELECTION_SECOND_PHY:
		val |= MDIO_CTL_REG_84823_MEDIA_COPPER_CORE_DOWN;
		initialize = 0;
		break;
	}
	if (params->phy[EXT_PHY2].req_line_speed == SPEED_1000)
		val |= MDIO_CTL_REG_84823_MEDIA_FIBER_1G;

	bnx2x_cl45_write(bp, phy, MDIO_CTL_DEVAD,
			 MDIO_CTL_REG_84823_MEDIA, val);
	DP(NETIF_MSG_LINK, "Multi_phy config = 0x%x, Media control = 0x%x\n",
		   params->multi_phy_config, val);

	if (bnx2x_is_8483x_8485x(phy)) {
		bnx2x_848xx_pair_swap_cfg(phy, params, vars);

		/* Keep AutogrEEEn disabled. */
		cmd_args[0] = 0x0;
		cmd_args[1] = 0x0;
		cmd_args[2] = PHY84833_CONSTANT_LATENCY + 1;
		cmd_args[3] = PHY84833_CONSTANT_LATENCY;
		rc = bnx2x_848xx_cmd_hdlr(phy, params,
					  PHY848xx_CMD_SET_EEE_MODE, cmd_args,
					  PHY848xx_CMDHDLR_MAX_ARGS);
		if (rc)
			DP(NETIF_MSG_LINK, "Cfg AutogrEEEn failed.\n");
	}
	if (initialize)
		rc = bnx2x_848xx_cmn_config_init(phy, params, vars);
	else
		bnx2x_save_848xx_spirom_version(phy, bp, params->port);
	/* 84833 PHY has a better feature and doesn't need to support this. */
	if (phy->type == PORT_HW_CFG_XGXS_EXT_PHY_TYPE_BCM84823) {
		u32 cms_enable = REG_RD(bp, params->shmem_base +
			offsetof(struct shmem_region,
			dev_info.port_hw_config[params->port].default_cfg)) &
			PORT_HW_CFG_ENABLE_CMS_MASK;

		bnx2x_cl45_read(bp, phy, MDIO_CTL_DEVAD,
				MDIO_CTL_REG_84823_USER_CTRL_REG, &val);
		if (cms_enable)
			val |= MDIO_CTL_REG_84823_USER_CTRL_CMS;
		else
			val &= ~MDIO_CTL_REG_84823_USER_CTRL_CMS;
		bnx2x_cl45_write(bp, phy, MDIO_CTL_DEVAD,
				 MDIO_CTL_REG_84823_USER_CTRL_REG, val);
	}

	bnx2x_cl45_read(bp, phy, MDIO_CTL_DEVAD,
			MDIO_84833_TOP_CFG_FW_REV, &val);

	/* Configure EEE support */
	if ((val >= MDIO_84833_TOP_CFG_FW_EEE) &&
	    (val != MDIO_84833_TOP_CFG_FW_NO_EEE) &&
	    bnx2x_eee_has_cap(params)) {
		rc = bnx2x_eee_initial_config(params, vars, SHMEM_EEE_10G_ADV);
		if (rc) {
			DP(NETIF_MSG_LINK, "Failed to configure EEE timers\n");
			bnx2x_8483x_disable_eee(phy, params, vars);
			return rc;
		}

		if ((phy->req_duplex == DUPLEX_FULL) &&
		    (params->eee_mode & EEE_MODE_ADV_LPI) &&
		    (bnx2x_eee_calc_timer(params) ||
		     !(params->eee_mode & EEE_MODE_ENABLE_LPI)))
			rc = bnx2x_8483x_enable_eee(phy, params, vars);
		else
			rc = bnx2x_8483x_disable_eee(phy, params, vars);
		if (rc) {
			DP(NETIF_MSG_LINK, "Failed to set EEE advertisement\n");
			return rc;
		}
	} else {
		vars->eee_status &= ~SHMEM_EEE_SUPPORTED_MASK;
	}

	if (phy->type == PORT_HW_CFG_XGXS_EXT_PHY_TYPE_BCM84833) {
		/* Additional settings for jumbo packets in 1000BASE-T mode */
		/* Allow rx extended length */
		bnx2x_cl45_read(bp, phy, MDIO_AN_DEVAD,
				MDIO_AN_REG_8481_AUX_CTRL, &val);
		val |= 0x4000;
		bnx2x_cl45_write(bp, phy, MDIO_AN_DEVAD,
				 MDIO_AN_REG_8481_AUX_CTRL, val);
		/* TX FIFO Elasticity LSB */
		bnx2x_cl45_read(bp, phy, MDIO_AN_DEVAD,
				MDIO_AN_REG_8481_1G_100T_EXT_CTRL, &val);
		val |= 0x1;
		bnx2x_cl45_write(bp, phy, MDIO_AN_DEVAD,
				 MDIO_AN_REG_8481_1G_100T_EXT_CTRL, val);
		/* TX FIFO Elasticity MSB */
		/* Enable expansion register 0x46 (Pattern Generator status) */
		bnx2x_cl45_write(bp, phy, MDIO_AN_DEVAD,
				 MDIO_AN_REG_8481_EXPANSION_REG_ACCESS, 0xf46);

		bnx2x_cl45_read(bp, phy, MDIO_AN_DEVAD,
				MDIO_AN_REG_8481_EXPANSION_REG_RD_RW, &val);
		val |= 0x4000;
		bnx2x_cl45_write(bp, phy, MDIO_AN_DEVAD,
				 MDIO_AN_REG_8481_EXPANSION_REG_RD_RW, val);
	}

	if (bnx2x_is_8483x_8485x(phy)) {
		/* Bring PHY out of super isolate mode as the final step. */
		bnx2x_cl45_read_and_write(bp, phy,
					  MDIO_CTL_DEVAD,
					  MDIO_84833_TOP_CFG_XGPHY_STRAP1,
					  (u16)~MDIO_84833_SUPER_ISOLATE);
	}
	return rc;
}
