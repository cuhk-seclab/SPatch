bool init_firmware(struct net_device *dev)
{
	struct r8192_priv	*priv = ieee80211_priv(dev);
	bool			rt_status = true;

	u32			file_length = 0;
	u8			*mapped_file = NULL;
	u32			init_step = 0;
	opt_rst_type_e	rst_opt = OPT_SYSTEM_RESET;
	firmware_init_step_e	starting_state = FW_INIT_STEP0_BOOT;

	rt_firmware		*pfirmware = priv->pFirmware;
	const struct firmware	*fw_entry;
	const char *fw_name[3] = { "RTL8192U/boot.img",
			   "RTL8192U/main.img",
			   "RTL8192U/data.img"};
	int rc;

	RT_TRACE(COMP_FIRMWARE, " PlatformInitFirmware()==>\n");

	if (pfirmware->firmware_status == FW_STATUS_0_INIT) {
		/* it is called by reset */
		rst_opt = OPT_SYSTEM_RESET;
		starting_state = FW_INIT_STEP0_BOOT;
		/* TODO: system reset */

	} else if (pfirmware->firmware_status == FW_STATUS_5_READY) {
		/* it is called by Initialize */
		rst_opt = OPT_FIRMWARE_RESET;
		starting_state = FW_INIT_STEP2_DATA;
	} else {
		 RT_TRACE(COMP_FIRMWARE, "PlatformInitFirmware: undefined firmware state\n");
	}

	/*
	 * Download boot, main, and data image for System reset.
	 * Download data image for firmware reset
	 */
	for (init_step = starting_state; init_step <= FW_INIT_STEP2_DATA; init_step++) {
		/*
		 * Open image file, and map file to continuous memory if open file success.
		 * or read image file from array. Default load from IMG file
		 */
		if (rst_opt == OPT_SYSTEM_RESET) {
			rc = request_firmware(&fw_entry, fw_name[init_step],&priv->udev->dev);
			if (rc < 0) {
				RT_TRACE(COMP_ERR, "request firmware fail!\n");
				goto download_firmware_fail;
			}

			if (fw_entry->size > sizeof(pfirmware->firmware_buf)) {
				RT_TRACE(COMP_ERR, "img file size exceed the container buffer fail!\n");
				goto download_firmware_fail;
			}

			if (init_step != FW_INIT_STEP1_MAIN) {
				memcpy(pfirmware->firmware_buf,fw_entry->data,fw_entry->size);
				mapped_file = pfirmware->firmware_buf;
				file_length = fw_entry->size;
			} else {
				memset(pfirmware->firmware_buf, 0, 128);
				memcpy(&pfirmware->firmware_buf[128],fw_entry->data,fw_entry->size);
				mapped_file = pfirmware->firmware_buf;
				file_length = fw_entry->size + 128;
			}
			pfirmware->firmware_buf_size = file_length;
		} else if (rst_opt == OPT_FIRMWARE_RESET) {
			/* we only need to download data.img here */
			mapped_file = pfirmware->firmware_buf;
			file_length = pfirmware->firmware_buf_size;
		}

		/* Download image file */
		/* The firmware download process is just as following,
		 * 1. that is each packet will be segmented and inserted to the wait queue.
		 * 2. each packet segment will be put in the skb_buff packet.
		 * 3. each skb_buff packet data content will already include the firmware info
		 *   and Tx descriptor info
		 * */
		rt_status = fw_download_code(dev, mapped_file, file_length);
		if (rst_opt == OPT_SYSTEM_RESET)
			release_firmware(fw_entry);

		if (rt_status != true)
			goto download_firmware_fail;

		switch (init_step) {
		case FW_INIT_STEP0_BOOT:
			/* Download boot
			 * initialize command descriptor.
			 * will set polling bit when firmware code is also configured
			 */
			pfirmware->firmware_status = FW_STATUS_1_MOVE_BOOT_CODE;
			/* mdelay(1000); */
			/*
			 * To initialize IMEM, CPU move code  from 0x80000080,
			 * hence, we send 0x80 byte packet
			 */
			break;

		case FW_INIT_STEP1_MAIN:
			/* Download firmware code. Wait until Boot Ready and Turn on CPU */
			pfirmware->firmware_status = FW_STATUS_2_MOVE_MAIN_CODE;

			/* Check Put Code OK and Turn On CPU */
			rt_status = CPUcheck_maincodeok_turnonCPU(dev);
			if (rt_status != true) {
				RT_TRACE(COMP_ERR, "CPUcheck_maincodeok_turnonCPU fail!\n");
				goto download_firmware_fail;
			}

			pfirmware->firmware_status = FW_STATUS_3_TURNON_CPU;
			break;

		case FW_INIT_STEP2_DATA:
			/* download initial data code */
			pfirmware->firmware_status = FW_STATUS_4_MOVE_DATA_CODE;
			mdelay(1);

			rt_status = CPUcheck_firmware_ready(dev);
			if (rt_status != true) {
				RT_TRACE(COMP_ERR, "CPUcheck_firmware_ready fail(%d)!\n",rt_status);
				goto download_firmware_fail;
			}

			/* wait until data code is initialized ready.*/
			pfirmware->firmware_status = FW_STATUS_5_READY;
			break;
		}
	}

	RT_TRACE(COMP_FIRMWARE, "Firmware Download Success\n");
	return rt_status;

download_firmware_fail:
	RT_TRACE(COMP_ERR, "ERR in %s()\n", __func__);
	rt_status = false;
	return rt_status;

}
