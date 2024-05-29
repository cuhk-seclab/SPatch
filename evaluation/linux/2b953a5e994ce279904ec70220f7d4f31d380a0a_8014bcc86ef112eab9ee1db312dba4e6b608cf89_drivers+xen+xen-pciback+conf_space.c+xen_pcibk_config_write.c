int xen_pcibk_config_write(struct pci_dev *dev, int offset, int size, u32 value)
{
	int err = 0, handled = 0;
	struct xen_pcibk_dev_data *dev_data = pci_get_drvdata(dev);
	const struct config_field_entry *cfg_entry;
	const struct config_field *field;
	u32 tmp_val;
	int req_start, req_end, field_start, field_end;

	if (unlikely(verbose_request))
		printk(KERN_DEBUG
		       DRV_NAME ": %s: write request %d bytes at 0x%x = %x\n",
		       pci_name(dev), size, offset, value);

	if (!valid_request(offset, size))
		return XEN_PCI_ERR_invalid_offset;

	list_for_each_entry(cfg_entry, &dev_data->config_fields, list) {
		field = cfg_entry->field;

		req_start = offset;
		req_end = offset + size;
		field_start = OFFSET(cfg_entry);
		field_end = OFFSET(cfg_entry) + field->size;

		if ((req_start >= field_start && req_start < field_end)
		    || (req_end > field_start && req_end <= field_end)) {
			tmp_val = 0;

			err = xen_pcibk_config_read(dev, field_start,
						  field->size, &tmp_val);
			if (err)
				break;

			tmp_val = merge_value(tmp_val, value, get_mask(size),
					      req_start - field_start);

			err = conf_space_write(dev, cfg_entry, field_start,
					       tmp_val);

			/* handled is set true here, but not every byte
			 * may have been written! Properly detecting if
			 * every byte is handled is unnecessary as the
			 * flag is used to detect devices that need
			 * special helpers to work correctly.
			 */
			handled = 1;
		}
	}

	if (!handled && !err) {
		/* By default, anything not specificially handled above is
		 * read-only. The permissive flag changes this behavior so
		 * that anything not specifically handled above is writable.
		 * This means that some fields may still be read-only because
		 * they have entries in the config_field list that intercept
		 * the write and do nothing. */
		if (dev_data->permissive || permissive) {
			switch (size) {
			case 1:
				err = pci_write_config_byte(dev, offset,
							    (u8) value);
				break;
			case 2:
				err = pci_write_config_word(dev, offset,
							    (u16) value);
				break;
			case 4:
				err = pci_write_config_dword(dev, offset,
							     (u32) value);
				break;
			}
		} else if (!dev_data->warned_on_write) {
			dev_data->warned_on_write = 1;
			dev_warn(&dev->dev, "Driver tried to write to a "
				 "read-only configuration space field at offset"
				 " 0x%x, size %d. This may be harmless, but if "
				 "you have problems with your device:\n"
				 "1) see permissive attribute in sysfs\n"
				 "2) report problems to the xen-devel "
				 "mailing list along with details of your "
				 "device obtained from lspci.\n", offset, size);
		}
	}

	return xen_pcibios_err_to_errno(err);
}
