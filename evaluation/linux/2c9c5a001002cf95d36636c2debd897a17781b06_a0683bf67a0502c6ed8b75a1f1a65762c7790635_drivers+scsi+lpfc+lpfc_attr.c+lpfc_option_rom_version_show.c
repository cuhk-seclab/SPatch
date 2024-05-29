static ssize_t
lpfc_option_rom_version_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	struct Scsi_Host  *shost = class_to_shost(dev);
	struct lpfc_vport *vport = (struct lpfc_vport *) shost->hostdata;
	struct lpfc_hba   *phba = vport->phba;
	char fwrev[FW_REV_STR_SIZE];

	if (phba->sli_rev < LPFC_SLI_REV4)
		return snprintf(buf, PAGE_SIZE, "%s\n", phba->OptionROMVersion);

	lpfc_decode_firmware_rev(phba, fwrev, 1);
	return snprintf(buf, PAGE_SIZE, "%s\n", fwrev);
}
