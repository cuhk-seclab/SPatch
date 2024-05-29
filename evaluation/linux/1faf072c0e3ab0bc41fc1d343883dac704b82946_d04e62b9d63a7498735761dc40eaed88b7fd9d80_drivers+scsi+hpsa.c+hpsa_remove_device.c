static void hpsa_remove_device(struct ctlr_info *h,
			struct hpsa_scsi_dev_t *device)
{
	struct scsi_device *sdev = NULL;

	if (!h->scsi_host)
		return;

	if (is_logical_device(device)) { /* RAID */
		sdev = scsi_device_lookup(h->scsi_host, device->bus,
						device->target, device->lun);
		if (sdev) {
			scsi_remove_device(sdev);
			scsi_device_put(sdev);
		} else {
			/*
			 * We don't expect to get here.  Future commands
			 * to this device will get a selection timeout as
			 * if the device were gone.
			 */
			hpsa_show_dev_msg(KERN_WARNING, h, device,
					"didn't find device for removal.");
		}
	} else /* HBA */
		hpsa_remove_sas_device(device);
}
