static int nvt_hw_detect(struct nvt_dev *nvt)
{
	u8 chip_major, chip_minor;
	char chip_id[12];
	bool chip_unknown = false;

	nvt_efm_enable(nvt);

	/* Check if we're wired for the alternate EFER setup */
	chip_major = nvt_cr_read(nvt, CR_CHIP_ID_HI);
	if (chip_major == 0xff) {
		nvt->cr_efir = CR_EFIR2;
		nvt->cr_efdr = CR_EFDR2;
		nvt_efm_enable(nvt);
		chip_major = nvt_cr_read(nvt, CR_CHIP_ID_HI);
	}

	chip_minor = nvt_cr_read(nvt, CR_CHIP_ID_LO);

	/* these are the known working chip revisions... */
	switch (chip_major) {
	case CHIP_ID_HIGH_667:
		strcpy(chip_id, "w83667hg\0");
		if (chip_minor != CHIP_ID_LOW_667)
			chip_unknown = true;
		break;
	case CHIP_ID_HIGH_677B:
		strcpy(chip_id, "w83677hg\0");
		if (chip_minor != CHIP_ID_LOW_677B2 &&
		    chip_minor != CHIP_ID_LOW_677B3)
			chip_unknown = true;
		break;
	case CHIP_ID_HIGH_677C:
		strcpy(chip_id, "w83677hg-c\0");
		if (chip_minor != CHIP_ID_LOW_677C)
			chip_unknown = true;
		break;
	default:
		strcpy(chip_id, "w836x7hg\0");
		chip_unknown = true;
		break;
	}

	/* warn, but still let the driver load, if we don't know this chip */
	if (chip_unknown)
		nvt_pr(KERN_WARNING, "%s: unknown chip, id: 0x%02x 0x%02x, "
		       "it may not work...", chip_id, chip_major, chip_minor);
	else
		nvt_dbg("%s: chip id: 0x%02x 0x%02x",
			chip_id, chip_major, chip_minor);

	nvt_efm_disable(nvt);

	nvt->chip_major = chip_major;
	nvt->chip_minor = chip_minor;

	return 0;
}
