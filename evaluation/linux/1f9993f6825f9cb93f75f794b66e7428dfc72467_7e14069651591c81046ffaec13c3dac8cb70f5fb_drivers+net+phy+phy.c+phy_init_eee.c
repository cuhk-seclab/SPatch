int phy_init_eee(struct phy_device *phydev, bool clk_stop_enable)
{
	/* According to 802.3az,the EEE is supported only in full duplex-mode.
	 * Also EEE feature is active when core is operating with MII, GMII
	 * or RGMII (all kinds). Internal PHYs are also allowed to proceed and
	 * should return an error if they do not support EEE.
	 */
	if ((phydev->duplex == DUPLEX_FULL) &&
	    ((phydev->interface == PHY_INTERFACE_MODE_MII) ||
	    (phydev->interface == PHY_INTERFACE_MODE_GMII) ||
	    (phydev->interface >= PHY_INTERFACE_MODE_RGMII &&
	     phydev->interface <= PHY_INTERFACE_MODE_RGMII_TXID) ||
	     phy_is_internal(phydev))) {
		int eee_lp, eee_cap, eee_adv;
		u32 lp, cap, adv;
		int status;

		/* Read phy status to properly get the right settings */
		status = phy_read_status(phydev);
		if (status)
			return status;

		/* First check if the EEE ability is supported */
		eee_cap = phy_read_mmd_indirect(phydev, MDIO_PCS_EEE_ABLE,
						MDIO_MMD_PCS, phydev->addr);
		if (eee_cap <= 0)
			goto eee_exit_err;

		cap = mmd_eee_cap_to_ethtool_sup_t(eee_cap);
		if (!cap)
			goto eee_exit_err;

		/* Check which link settings negotiated and verify it in
		 * the EEE advertising registers.
		 */
		eee_lp = phy_read_mmd_indirect(phydev, MDIO_AN_EEE_LPABLE,
					       MDIO_MMD_AN, phydev->addr);
		if (eee_lp <= 0)
			goto eee_exit_err;

		eee_adv = phy_read_mmd_indirect(phydev, MDIO_AN_EEE_ADV,
						MDIO_MMD_AN, phydev->addr);
		if (eee_adv <= 0)
			goto eee_exit_err;

		adv = mmd_eee_adv_to_ethtool_adv_t(eee_adv);
		lp = mmd_eee_adv_to_ethtool_adv_t(eee_lp);
		if (!phy_check_valid(phydev->speed, phydev->duplex, lp & adv))
			goto eee_exit_err;

		if (clk_stop_enable) {
			/* Configure the PHY to stop receiving xMII
			 * clock while it is signaling LPI.
			 */
			int val = phy_read_mmd_indirect(phydev, MDIO_CTRL1,
							MDIO_MMD_PCS,
							phydev->addr);
			if (val < 0)
				return val;

			val |= MDIO_PCS_CTRL1_CLKSTOP_EN;
			phy_write_mmd_indirect(phydev, MDIO_CTRL1,
					       MDIO_MMD_PCS, phydev->addr,
					       val);
		}

		return 0; /* EEE supported */
	}
eee_exit_err:
	return -EPROTONOSUPPORT;
}
