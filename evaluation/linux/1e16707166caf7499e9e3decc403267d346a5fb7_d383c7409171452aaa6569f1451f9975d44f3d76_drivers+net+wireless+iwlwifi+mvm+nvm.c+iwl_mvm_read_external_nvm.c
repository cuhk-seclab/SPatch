static int iwl_mvm_read_external_nvm(struct iwl_mvm *mvm)
{
	int ret, section_size;
	u16 section_id;
	const struct firmware *fw_entry;
	const struct {
		__le16 word1;
		__le16 word2;
		u8 data[];
	} *file_sec;
	const u8 *eof, *temp;
	int max_section_size;
	const __le32 *dword_buff;

#define NVM_WORD1_LEN(x) (8 * (x & 0x03FF))
#define NVM_WORD2_ID(x) (x >> 12)
#define NVM_WORD2_LEN_FAMILY_8000(x) (2 * ((x & 0xFF) << 8 | x >> 8))
#define NVM_WORD1_ID_FAMILY_8000(x) (x >> 4)
#define NVM_HEADER_0	(0x2A504C54)
#define NVM_HEADER_1	(0x4E564D2A)
#define NVM_HEADER_SIZE	(4 * sizeof(u32))

	IWL_DEBUG_EEPROM(mvm->trans->dev, "Read from external NVM\n");

	/* Maximal size depends on HW family and step */
	if (mvm->trans->cfg->device_family != IWL_DEVICE_FAMILY_8000)
		max_section_size = IWL_MAX_NVM_SECTION_SIZE;
	else
		max_section_size = IWL_MAX_NVM_8000_SECTION_SIZE;

	/*
	 * Obtain NVM image via request_firmware. Since we already used
	 * request_firmware_nowait() for the firmware binary load and only
	 * get here after that we assume the NVM request can be satisfied
	 * synchronously.
	 */
	ret = request_firmware(&fw_entry, mvm->nvm_file_name,
			       mvm->trans->dev);
	if (ret) {
		IWL_ERR(mvm, "ERROR: %s isn't available %d\n",
			mvm->nvm_file_name, ret);
		return ret;
	}

	IWL_INFO(mvm, "Loaded NVM file %s (%zu bytes)\n",
		 mvm->nvm_file_name, fw_entry->size);

	if (fw_entry->size > MAX_NVM_FILE_LEN) {
		IWL_ERR(mvm, "NVM file too large\n");
		ret = -EINVAL;
		goto out;
	}

	eof = fw_entry->data + fw_entry->size;
	dword_buff = (__le32 *)fw_entry->data;

	/* some NVM file will contain a header.
	 * The header is identified by 2 dwords header as follow:
	 * dword[0] = 0x2A504C54
	 * dword[1] = 0x4E564D2A
	 *
	 * This header must be skipped when providing the NVM data to the FW.
	 */
	if (fw_entry->size > NVM_HEADER_SIZE &&
	    dword_buff[0] == cpu_to_le32(NVM_HEADER_0) &&
	    dword_buff[1] == cpu_to_le32(NVM_HEADER_1)) {
		file_sec = (void *)(fw_entry->data + NVM_HEADER_SIZE);
		IWL_INFO(mvm, "NVM Version %08X\n", le32_to_cpu(dword_buff[2]));
		IWL_INFO(mvm, "NVM Manufacturing date %08X\n",
			 le32_to_cpu(dword_buff[3]));

		/* nvm file validation, dword_buff[2] holds the file version */
		if ((CSR_HW_REV_STEP(mvm->trans->hw_rev) == SILICON_C_STEP &&
		     le32_to_cpu(dword_buff[2]) < 0xE4A) ||
		    (CSR_HW_REV_STEP(mvm->trans->hw_rev) == SILICON_B_STEP &&
		     le32_to_cpu(dword_buff[2]) >= 0xE4A)) {
			ret = -EFAULT;
			goto out;
		}
	} else {
		file_sec = (void *)fw_entry->data;
	}

	while (true) {
		if (file_sec->data > eof) {
			IWL_ERR(mvm,
				"ERROR - NVM file too short for section header\n");
			ret = -EINVAL;
			break;
		}

		/* check for EOF marker */
		if (!file_sec->word1 && !file_sec->word2) {
			ret = 0;
			break;
		}

		if (mvm->trans->cfg->device_family != IWL_DEVICE_FAMILY_8000) {
			section_size =
				2 * NVM_WORD1_LEN(le16_to_cpu(file_sec->word1));
			section_id = NVM_WORD2_ID(le16_to_cpu(file_sec->word2));
		} else {
			section_size = 2 * NVM_WORD2_LEN_FAMILY_8000(
						le16_to_cpu(file_sec->word2));
			section_id = NVM_WORD1_ID_FAMILY_8000(
						le16_to_cpu(file_sec->word1));
		}

		if (section_size > max_section_size) {
			IWL_ERR(mvm, "ERROR - section too large (%d)\n",
				section_size);
			ret = -EINVAL;
			break;
		}

		if (!section_size) {
			IWL_ERR(mvm, "ERROR - section empty\n");
			ret = -EINVAL;
			break;
		}

		if (file_sec->data + section_size > eof) {
			IWL_ERR(mvm,
				"ERROR - NVM file too short for section (%d bytes)\n",
				section_size);
			ret = -EINVAL;
			break;
		}

		if (WARN(section_id >= NVM_MAX_NUM_SECTIONS,
			 "Invalid NVM section ID %d\n", section_id)) {
			ret = -EINVAL;
			break;
		}

		temp = kmemdup(file_sec->data, section_size, GFP_KERNEL);
		if (!temp) {
			ret = -ENOMEM;
			break;
		}
		mvm->nvm_sections[section_id].data = temp;
		mvm->nvm_sections[section_id].length = section_size;

		/* advance to the next section */
		file_sec = (void *)(file_sec->data + section_size);
	}
out:
	release_firmware(fw_entry);
	return ret;
}
