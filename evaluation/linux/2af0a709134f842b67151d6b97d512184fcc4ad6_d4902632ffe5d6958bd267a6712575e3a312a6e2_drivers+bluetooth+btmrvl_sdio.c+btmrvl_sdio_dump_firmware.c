static void btmrvl_sdio_dump_firmware(struct btmrvl_private *priv)
{
	struct btmrvl_sdio_card *card = priv->btmrvl_dev.card;
	int ret = 0;
	unsigned int reg, reg_start, reg_end;
	enum rdwr_status stat;
	u8 *dbg_ptr, *end_ptr, *fw_dump_data, *fw_dump_ptr;
	u8 dump_num = 0, idx, i, read_reg, doneflag = 0;
	u32 memory_size, fw_dump_len = 0;

	/* dump sdio register first */
	btmrvl_sdio_dump_regs(priv);

	if (!card->supports_fw_dump) {
		BT_ERR("Firmware dump not supported for this card!");
		return;
	}

	for (idx = 0; idx < ARRAY_SIZE(mem_type_mapping_tbl); idx++) {
		struct memory_type_mapping *entry = &mem_type_mapping_tbl[idx];

		if (entry->mem_ptr) {
			vfree(entry->mem_ptr);
			entry->mem_ptr = NULL;
		}
		entry->mem_size = 0;
	}

	btmrvl_sdio_wakeup_fw(priv);
	sdio_claim_host(card->func);

	BT_INFO("== btmrvl firmware dump start ==");

	stat = btmrvl_sdio_rdwr_firmware(priv, doneflag);
	if (stat == RDWR_STATUS_FAILURE)
		goto done;

	reg = card->reg->fw_dump_start;
	/* Read the number of the memories which will dump */
	dump_num = sdio_readb(card->func, reg, &ret);

	if (ret) {
		BT_ERR("SDIO read memory length err");
		goto done;
	}

	/* Read the length of every memory which will dump */
	for (idx = 0; idx < dump_num; idx++) {
		struct memory_type_mapping *entry = &mem_type_mapping_tbl[idx];

		stat = btmrvl_sdio_rdwr_firmware(priv, doneflag);
		if (stat == RDWR_STATUS_FAILURE)
			goto done;

		memory_size = 0;
		reg = card->reg->fw_dump_start;
		for (i = 0; i < 4; i++) {
			read_reg = sdio_readb(card->func, reg, &ret);
			if (ret) {
				BT_ERR("SDIO read err");
				goto done;
			}
			memory_size |= (read_reg << i*8);
			reg++;
		}

		if (memory_size == 0) {
			BT_INFO("Firmware dump finished!");
			break;
		}

		BT_INFO("%s_SIZE=0x%x", entry->mem_name, memory_size);
		entry->mem_ptr = vzalloc(memory_size + 1);
		entry->mem_size = memory_size;
		if (!entry->mem_ptr) {
			BT_ERR("Vzalloc %s failed", entry->mem_name);
			goto done;
		}

		fw_dump_len += (strlen("========Start dump ") +
				strlen(entry->mem_name) +
				strlen("========\n") +
				(memory_size + 1) +
				strlen("\n========End dump========\n"));

		dbg_ptr = entry->mem_ptr;
		end_ptr = dbg_ptr + memory_size;

		doneflag = entry->done_flag;
		BT_INFO("Start %s output, please wait...",
			entry->mem_name);

		do {
			stat = btmrvl_sdio_rdwr_firmware(priv, doneflag);
			if (stat == RDWR_STATUS_FAILURE)
				goto done;

			reg_start = card->reg->fw_dump_start;
			reg_end = card->reg->fw_dump_end;
			for (reg = reg_start; reg <= reg_end; reg++) {
				*dbg_ptr = sdio_readb(card->func, reg, &ret);
				if (ret) {
					BT_ERR("SDIO read err");
					goto done;
				}
				if (dbg_ptr < end_ptr)
					dbg_ptr++;
				else
					BT_ERR("Allocated buffer not enough");
			}

			if (stat != RDWR_STATUS_DONE) {
				continue;
			} else {
				BT_INFO("%s done: size=0x%tx",
					entry->mem_name,
					dbg_ptr - entry->mem_ptr);
				break;
			}
		} while (1);
	}

	BT_INFO("== btmrvl firmware dump end ==");

done:
	sdio_release_host(card->func);

	if (fw_dump_len == 0)
		return;

	fw_dump_data = vzalloc(fw_dump_len+1);
	if (!fw_dump_data) {
		BT_ERR("Vzalloc fw_dump_data fail!");
		return;
	}
	fw_dump_ptr = fw_dump_data;

	/* Dump all the memory data into single file, a userspace script will
	   be used to split all the memory data to multiple files*/
	BT_INFO("== btmrvl firmware dump to /sys/class/devcoredump start");
	for (idx = 0; idx < dump_num; idx++) {
		struct memory_type_mapping *entry = &mem_type_mapping_tbl[idx];

		if (entry->mem_ptr) {
			strcpy(fw_dump_ptr, "========Start dump ");
			fw_dump_ptr += strlen("========Start dump ");

			strcpy(fw_dump_ptr, entry->mem_name);
			fw_dump_ptr += strlen(entry->mem_name);

			strcpy(fw_dump_ptr, "========\n");
			fw_dump_ptr += strlen("========\n");

			memcpy(fw_dump_ptr, entry->mem_ptr, entry->mem_size);
			fw_dump_ptr += entry->mem_size;

			strcpy(fw_dump_ptr, "\n========End dump========\n");
			fw_dump_ptr += strlen("\n========End dump========\n");

			vfree(mem_type_mapping_tbl[idx].mem_ptr);
			mem_type_mapping_tbl[idx].mem_ptr = NULL;
		}
	}

	/* fw_dump_data will be free in device coredump release function
	   after 5 min*/
	dev_coredumpv(&card->func->dev, fw_dump_data, fw_dump_len, GFP_KERNEL);
	BT_INFO("== btmrvl firmware dump to /sys/class/devcoredump end");
}
