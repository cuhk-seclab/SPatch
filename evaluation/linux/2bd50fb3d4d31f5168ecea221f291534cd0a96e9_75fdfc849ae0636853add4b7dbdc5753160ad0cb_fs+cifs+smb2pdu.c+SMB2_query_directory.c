int
SMB2_query_directory(const unsigned int xid, struct cifs_tcon *tcon,
		     u64 persistent_fid, u64 volatile_fid, int index,
		     struct cifs_search_info *srch_inf)
{
	struct smb2_query_directory_req *req;
	struct smb2_query_directory_rsp *rsp = NULL;
	struct kvec iov[2];
	int rc = 0;
	int len;
	int resp_buftype = CIFS_NO_BUFFER;
	unsigned char *bufptr;
	struct TCP_Server_Info *server;
	struct cifs_ses *ses = tcon->ses;
	__le16 asteriks = cpu_to_le16('*');
	char *end_of_smb;
	unsigned int output_size = CIFSMaxBufSize;
	size_t info_buf_size;

	if (ses && (ses->server))
		server = ses->server;
	else
		return -EIO;

	rc = small_smb2_init(SMB2_QUERY_DIRECTORY, tcon, (void **) &req);
	if (rc)
		return rc;

	switch (srch_inf->info_level) {
	case SMB_FIND_FILE_DIRECTORY_INFO:
		req->FileInformationClass = FILE_DIRECTORY_INFORMATION;
		info_buf_size = sizeof(FILE_DIRECTORY_INFO) - 1;
		break;
	case SMB_FIND_FILE_ID_FULL_DIR_INFO:
		req->FileInformationClass = FILEID_FULL_DIRECTORY_INFORMATION;
		info_buf_size = sizeof(SEARCH_ID_FULL_DIR_INFO) - 1;
		break;
	default:
		cifs_dbg(VFS, "info level %u isn't supported\n",
			 srch_inf->info_level);
		rc = -EINVAL;
		goto qdir_exit;
	}

	req->FileIndex = cpu_to_le32(index);
	req->PersistentFileId = persistent_fid;
	req->VolatileFileId = volatile_fid;

	len = 0x2;
	bufptr = req->Buffer;
	memcpy(bufptr, &asteriks, len);

	req->FileNameOffset =
		cpu_to_le16(sizeof(struct smb2_query_directory_req) - 1 - 4);
	req->FileNameLength = cpu_to_le16(len);
	/*
	 * BB could be 30 bytes or so longer if we used SMB2 specific
	 * buffer lengths, but this is safe and close enough.
	 */
	output_size = min_t(unsigned int, output_size, server->maxBuf);
	output_size = min_t(unsigned int, output_size, 2 << 15);
	req->OutputBufferLength = cpu_to_le32(output_size);

	iov[0].iov_base = (char *)req;
	/* 4 for RFC1001 length and 1 for Buffer */
	iov[0].iov_len = get_rfc1002_length(req) + 4 - 1;

	iov[1].iov_base = (char *)(req->Buffer);
	iov[1].iov_len = len;

	inc_rfc1001_len(req, len - 1 /* Buffer */);

	rc = SendReceive2(xid, ses, iov, 2, &resp_buftype, 0);
	rsp = (struct smb2_query_directory_rsp *)iov[0].iov_base;

	if (rc) {
		if (rc == -ENODATA && rsp->hdr.Status == STATUS_NO_MORE_FILES) {
			srch_inf->endOfSearch = true;
			rc = 0;
		}
		cifs_stats_fail_inc(tcon, SMB2_QUERY_DIRECTORY_HE);
		goto qdir_exit;
	}

	rc = validate_buf(le16_to_cpu(rsp->OutputBufferOffset),
			  le32_to_cpu(rsp->OutputBufferLength), &rsp->hdr,
			  info_buf_size);
	if (rc)
		goto qdir_exit;

	srch_inf->unicode = true;

	if (srch_inf->ntwrk_buf_start) {
		if (srch_inf->smallBuf)
			cifs_small_buf_release(srch_inf->ntwrk_buf_start);
		else
			cifs_buf_release(srch_inf->ntwrk_buf_start);
	}
	srch_inf->ntwrk_buf_start = (char *)rsp;
	srch_inf->srch_entries_start = srch_inf->last_entry = 4 /* rfclen */ +
		(char *)&rsp->hdr + le16_to_cpu(rsp->OutputBufferOffset);
	/* 4 for rfc1002 length field */
	end_of_smb = get_rfc1002_length(rsp) + 4 + (char *)&rsp->hdr;
	srch_inf->entries_in_buffer =
			num_entries(srch_inf->srch_entries_start, end_of_smb,
				    &srch_inf->last_entry, info_buf_size);
	srch_inf->index_of_last_entry += srch_inf->entries_in_buffer;
	cifs_dbg(FYI, "num entries %d last_index %lld srch start %p srch end %p\n",
		 srch_inf->entries_in_buffer, srch_inf->index_of_last_entry,
		 srch_inf->srch_entries_start, srch_inf->last_entry);
	if (resp_buftype == CIFS_LARGE_BUFFER)
		srch_inf->smallBuf = false;
	else if (resp_buftype == CIFS_SMALL_BUFFER)
		srch_inf->smallBuf = true;
	else
		cifs_dbg(VFS, "illegal search buffer type\n");

	return rc;

qdir_exit:
	free_rsp_buf(resp_buftype, rsp);
	return rc;
}
