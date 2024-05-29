static int brcmf_pcie_get_fwnames(struct brcmf_pciedev_info *devinfo)
{
	char *fw_name;
	char *nvram_name;
	uint fw_len, nv_len;
	char end;

	brcmf_dbg(PCIE, "Enter, chip 0x%04x chiprev %d\n", devinfo->ci->chip,
		  devinfo->ci->chiprev);

	switch (devinfo->ci->chip) {
	case BRCM_CC_43602_CHIP_ID:
		fw_name = BRCMF_PCIE_43602_FW_NAME;
		nvram_name = BRCMF_PCIE_43602_NVRAM_NAME;
		break;
	case BRCM_CC_4350_CHIP_ID:
		fw_name = BRCMF_PCIE_4350_FW_NAME;
		nvram_name = BRCMF_PCIE_4350_NVRAM_NAME;
		break;
	case BRCM_CC_4356_CHIP_ID:
		fw_name = BRCMF_PCIE_4356_FW_NAME;
		nvram_name = BRCMF_PCIE_4356_NVRAM_NAME;
		break;
	case BRCM_CC_43567_CHIP_ID:
	case BRCM_CC_43569_CHIP_ID:
	case BRCM_CC_43570_CHIP_ID:
		fw_name = BRCMF_PCIE_43570_FW_NAME;
		nvram_name = BRCMF_PCIE_43570_NVRAM_NAME;
		break;
	case BRCM_CC_4358_CHIP_ID:
		fw_name = BRCMF_PCIE_4358_FW_NAME;
		nvram_name = BRCMF_PCIE_4358_NVRAM_NAME;
		break;
	default:
		brcmf_err("Unsupported chip 0x%04x\n", devinfo->ci->chip);
		return -ENODEV;
	}

	fw_len = sizeof(devinfo->fw_name) - 1;
	nv_len = sizeof(devinfo->nvram_name) - 1;
	/* check if firmware path is provided by module parameter */
	if (brcmf_firmware_path[0] != '\0') {
		strncpy(devinfo->fw_name, brcmf_firmware_path, fw_len);
		strncpy(devinfo->nvram_name, brcmf_firmware_path, nv_len);
		fw_len -= strlen(devinfo->fw_name);
		nv_len -= strlen(devinfo->nvram_name);

		end = brcmf_firmware_path[strlen(brcmf_firmware_path) - 1];
		if (end != '/') {
			strncat(devinfo->fw_name, "/", fw_len);
			strncat(devinfo->nvram_name, "/", nv_len);
			fw_len--;
			nv_len--;
		}
	}
	strncat(devinfo->fw_name, fw_name, fw_len);
	strncat(devinfo->nvram_name, nvram_name, nv_len);

	return 0;
}
