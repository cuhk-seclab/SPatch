static u32 brcmf_chip_tcm_rambase(struct brcmf_chip_priv *ci)
{
	switch (ci->pub.chip) {
	case BRCM_CC_4345_CHIP_ID:
		return 0x198000;
	case BRCM_CC_4335_CHIP_ID:
	case BRCM_CC_4339_CHIP_ID:
	case BRCM_CC_4350_CHIP_ID:
	case BRCM_CC_4354_CHIP_ID:
	case BRCM_CC_4356_CHIP_ID:
	case BRCM_CC_43567_CHIP_ID:
	case BRCM_CC_43569_CHIP_ID:
	case BRCM_CC_43570_CHIP_ID:
	case BRCM_CC_4358_CHIP_ID:
	case BRCM_CC_43602_CHIP_ID:
		return 0x180000;
	default:
		brcmf_err("unknown chip: %s\n", ci->pub.name);
		break;
	}
	return 0;
}
