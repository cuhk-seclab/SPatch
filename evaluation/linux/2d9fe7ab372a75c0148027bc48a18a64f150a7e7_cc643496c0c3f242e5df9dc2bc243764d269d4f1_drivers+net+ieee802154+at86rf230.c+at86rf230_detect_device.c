static int
at86rf230_detect_device(struct at86rf230_local *lp)
{
	unsigned int part, version, val;
	u16 man_id = 0;
	const char *chip;
	int rc;

	rc = __at86rf230_read(lp, RG_MAN_ID_0, &val);
	if (rc)
		return rc;
	man_id |= val;

	rc = __at86rf230_read(lp, RG_MAN_ID_1, &val);
	if (rc)
		return rc;
	man_id |= (val << 8);

	rc = __at86rf230_read(lp, RG_PART_NUM, &part);
	if (rc)
		return rc;

	rc = __at86rf230_read(lp, RG_VERSION_NUM, &version);
	if (rc)
		return rc;

	if (man_id != 0x001f) {
		dev_err(&lp->spi->dev, "Non-Atmel dev found (MAN_ID %02x %02x)\n",
			man_id >> 8, man_id & 0xFF);
		return -EINVAL;
	}

	lp->hw->flags = IEEE802154_HW_TX_OMIT_CKSUM | IEEE802154_HW_AACK |
			IEEE802154_HW_CSMA_PARAMS |
			IEEE802154_HW_FRAME_RETRIES | IEEE802154_HW_AFILT |
			IEEE802154_HW_PROMISCUOUS;

	lp->hw->phy->flags = WPAN_PHY_FLAG_TXPOWER |
			     WPAN_PHY_FLAG_CCA_ED_LEVEL |
			     WPAN_PHY_FLAG_CCA_MODE;

	lp->hw->phy->supported.cca_modes = BIT(NL802154_CCA_ENERGY) |
		BIT(NL802154_CCA_CARRIER) | BIT(NL802154_CCA_ENERGY_CARRIER);
	lp->hw->phy->supported.cca_opts = BIT(NL802154_CCA_OPT_ENERGY_CARRIER_AND) |
		BIT(NL802154_CCA_OPT_ENERGY_CARRIER_OR);

	lp->hw->phy->supported.cca_ed_levels = at86rf23x_ed_levels;
	lp->hw->phy->supported.cca_ed_levels_size = ARRAY_SIZE(at86rf23x_ed_levels);

	lp->hw->phy->cca.mode = NL802154_CCA_ENERGY;

	switch (part) {
	case 2:
		chip = "at86rf230";
		rc = -ENOTSUPP;
		goto not_supp;
	case 3:
		chip = "at86rf231";
		lp->data = &at86rf231_data;
		lp->hw->phy->supported.channels[0] = 0x7FFF800;
		lp->hw->phy->current_channel = 11;
		lp->hw->phy->symbol_duration = 16;
		lp->hw->phy->supported.tx_powers = at86rf231_powers;
		lp->hw->phy->supported.tx_powers_size = ARRAY_SIZE(at86rf231_powers);
		break;
	case 7:
		chip = "at86rf212";
		lp->data = &at86rf212_data;
		lp->hw->flags |= IEEE802154_HW_LBT;
		lp->hw->phy->supported.channels[0] = 0x00007FF;
		lp->hw->phy->supported.channels[2] = 0x00007FF;
		lp->hw->phy->current_channel = 5;
		lp->hw->phy->symbol_duration = 25;
		lp->hw->phy->supported.lbt = NL802154_SUPPORTED_BOOL_BOTH;
		lp->hw->phy->supported.tx_powers = at86rf212_powers;
		lp->hw->phy->supported.tx_powers_size = ARRAY_SIZE(at86rf212_powers);
		lp->hw->phy->supported.cca_ed_levels = at86rf212_ed_levels_100;
		lp->hw->phy->supported.cca_ed_levels_size = ARRAY_SIZE(at86rf212_ed_levels_100);
		break;
	case 11:
		chip = "at86rf233";
		lp->data = &at86rf233_data;
		lp->hw->phy->supported.channels[0] = 0x7FFF800;
		lp->hw->phy->current_channel = 13;
		lp->hw->phy->symbol_duration = 16;
		lp->hw->phy->supported.tx_powers = at86rf233_powers;
		lp->hw->phy->supported.tx_powers_size = ARRAY_SIZE(at86rf233_powers);
		break;
	default:
		chip = "unknown";
		rc = -ENOTSUPP;
		goto not_supp;
	}

	lp->hw->phy->cca_ed_level = lp->hw->phy->supported.cca_ed_levels[7];

not_supp:
	dev_info(&lp->spi->dev, "Detected %s chip version %d\n", chip, version);

	return rc;
}
