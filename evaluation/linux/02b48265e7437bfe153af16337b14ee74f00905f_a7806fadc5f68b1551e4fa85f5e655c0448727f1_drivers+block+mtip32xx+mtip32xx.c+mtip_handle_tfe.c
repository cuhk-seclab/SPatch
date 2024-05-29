static void mtip_handle_tfe(struct driver_data *dd)
{
	int group, tag, bit, reissue, rv;
	struct mtip_port *port;
	struct mtip_cmd  *cmd;
	u32 completed;
	struct host_to_dev_fis *fis;
	unsigned long tagaccum[SLOTBITS_IN_LONGS];
	unsigned int cmd_cnt = 0;
	unsigned char *buf;
	char *fail_reason = NULL;
	int fail_all_ncq_write = 0, fail_all_ncq_cmds = 0;

	dev_warn(&dd->pdev->dev, "Taskfile error\n");

	port = dd->port;

	set_bit(MTIP_PF_EH_ACTIVE_BIT, &port->flags);

	if (test_bit(MTIP_PF_IC_ACTIVE_BIT, &port->flags)) {
		cmd = mtip_cmd_from_tag(dd, MTIP_TAG_INTERNAL);
		dbg_printk(MTIP_DRV_NAME " TFE for the internal command\n");

		if (cmd->comp_data && cmd->comp_func) {
			cmd->comp_func(port, MTIP_TAG_INTERNAL,
					cmd, PORT_IRQ_TF_ERR);
		}
		goto handle_tfe_exit;
	}

	/* clear the tag accumulator */
	memset(tagaccum, 0, SLOTBITS_IN_LONGS * sizeof(long));

	/* Loop through all the groups */
	for (group = 0; group < dd->slot_groups; group++) {
		completed = readl(port->completed[group]);

		dev_warn(&dd->pdev->dev, "g=%u, comp=%x\n", group, completed);

		/* clear completed status register in the hardware.*/
		writel(completed, port->completed[group]);

		/* Process successfully completed commands */
		for (bit = 0; bit < 32 && completed; bit++) {
			if (!(completed & (1<<bit)))
				continue;
			tag = (group << 5) + bit;

			/* Skip the internal command slot */
			if (tag == MTIP_TAG_INTERNAL)
				continue;

			cmd = mtip_cmd_from_tag(dd, tag);
			if (likely(cmd->comp_func)) {
				set_bit(tag, tagaccum);
				cmd_cnt++;
				cmd->comp_func(port, tag, cmd, 0);
			} else {
				dev_err(&port->dd->pdev->dev,
					"Missing completion func for tag %d",
					tag);
				if (mtip_check_surprise_removal(dd->pdev)) {
					/* don't proceed further */
					return;
				}
			}
		}
	}

	print_tags(dd, "completed (TFE)", tagaccum, cmd_cnt);

	/* Restart the port */
	mdelay(20);
	mtip_restart_port(port);

	/* Trying to determine the cause of the error */
	rv = mtip_read_log_page(dd->port, ATA_LOG_SATA_NCQ,
				dd->port->log_buf,
				dd->port->log_buf_dma, 1);
	if (rv) {
		dev_warn(&dd->pdev->dev,
			"Error in READ LOG EXT (10h) command\n");
		/* non-critical error, don't fail the load */
	} else {
		buf = (unsigned char *)dd->port->log_buf;
		if (buf[259] & 0x1) {
			dev_info(&dd->pdev->dev,
				"Write protect bit is set.\n");
			set_bit(MTIP_DDF_WRITE_PROTECT_BIT, &dd->dd_flag);
			fail_all_ncq_write = 1;
			fail_reason = "write protect";
		}
		if (buf[288] == 0xF7) {
			dev_info(&dd->pdev->dev,
				"Exceeded Tmax, drive in thermal shutdown.\n");
			set_bit(MTIP_DDF_OVER_TEMP_BIT, &dd->dd_flag);
			fail_all_ncq_cmds = 1;
			fail_reason = "thermal shutdown";
		}
		if (buf[288] == 0xBF) {
			set_bit(MTIP_DDF_SEC_LOCK_BIT, &dd->dd_flag);
			dev_info(&dd->pdev->dev,
				"Drive indicates rebuild has failed. Secure erase required.\n");
			fail_all_ncq_cmds = 1;
			fail_reason = "rebuild failed";
		}
	}

	/* clear the tag accumulator */
	memset(tagaccum, 0, SLOTBITS_IN_LONGS * sizeof(long));

	/* Loop through all the groups */
	for (group = 0; group < dd->slot_groups; group++) {
		for (bit = 0; bit < 32; bit++) {
			reissue = 1;
			tag = (group << 5) + bit;
			cmd = mtip_cmd_from_tag(dd, tag);

			fis = (struct host_to_dev_fis *)cmd->command;

			/* Should re-issue? */
			if (tag == MTIP_TAG_INTERNAL ||
			    fis->command == ATA_CMD_SET_FEATURES)
				reissue = 0;
			else {
				if (fail_all_ncq_cmds ||
					(fail_all_ncq_write &&
					fis->command == ATA_CMD_FPDMA_WRITE)) {
					dev_warn(&dd->pdev->dev,
					"  Fail: %s w/tag %d [%s].\n",
					fis->command == ATA_CMD_FPDMA_WRITE ?
						"write" : "read",
					tag,
					fail_reason != NULL ?
						fail_reason : "unknown");
					if (cmd->comp_func) {
						cmd->comp_func(port, tag,
							cmd, -ENODATA);
					}
					continue;
				}
			}

			/*
			 * First check if this command has
			 *  exceeded its retries.
			 */
			if (reissue && (cmd->retries-- > 0)) {

				set_bit(tag, tagaccum);

				/* Re-issue the command. */
				mtip_issue_ncq_command(port, tag);

				continue;
			}

			/* Retire a command that will not be reissued */
			dev_warn(&port->dd->pdev->dev,
				"retiring tag %d\n", tag);

			if (cmd->comp_func)
				cmd->comp_func(port, tag, cmd, PORT_IRQ_TF_ERR);
			else
				dev_warn(&port->dd->pdev->dev,
					"Bad completion for tag %d\n",
					tag);
		}
	}
	print_tags(dd, "reissued (TFE)", tagaccum, cmd_cnt);

handle_tfe_exit:
	/* clear eh_active */
	clear_bit(MTIP_PF_EH_ACTIVE_BIT, &port->flags);
	wake_up_interruptible(&port->svc_wait);
}
