static int
archive_read_format_zip_seekable_read_header(struct archive_read *a,
	struct archive_entry *entry)
{
	struct zip *zip = (struct zip *)a->format->data;
	struct zip_entry *rsrc;
	int64_t offset;
	int r, ret = ARCHIVE_OK;

	/*
	 * It should be sufficient to call archive_read_next_header() for
	 * a reader to determine if an entry is encrypted or not. If the
	 * encryption of an entry is only detectable when calling
	 * archive_read_data(), so be it. We'll do the same check there
	 * as well.
	 */
	if (zip->has_encrypted_entries ==
			ARCHIVE_READ_FORMAT_ENCRYPTION_DONT_KNOW)
		zip->has_encrypted_entries = 0;

	a->archive.archive_format = ARCHIVE_FORMAT_ZIP;
	if (a->archive.archive_format_name == NULL)
		a->archive.archive_format_name = "ZIP";

	if (zip->zip_entries == NULL) {
		r = slurp_central_directory(a, entry, zip);
		if (r != ARCHIVE_OK)
			return r;
		/* Get first entry whose local header offset is lower than
		 * other entries in the archive file. */
		zip->entry =
		    (struct zip_entry *)ARCHIVE_RB_TREE_MIN(&zip->tree);
	} else if (zip->entry != NULL) {
		/* Get next entry in local header offset order. */
		zip->entry = (struct zip_entry *)__archive_rb_tree_iterate(
		    &zip->tree, &zip->entry->node, ARCHIVE_RB_DIR_RIGHT);
	}

	if (zip->entry == NULL)
		return ARCHIVE_EOF;

	if (zip->entry->rsrcname.s)
		rsrc = (struct zip_entry *)__archive_rb_tree_find_node(
		    &zip->tree_rsrc, zip->entry->rsrcname.s);
	else
		rsrc = NULL;

	if (zip->cctx_valid)
		archive_decrypto_aes_ctr_release(&zip->cctx);
	if (zip->hctx_valid)
		archive_hmac_sha1_cleanup(&zip->hctx);
	zip->tctx_valid = zip->cctx_valid = zip->hctx_valid = 0;
	__archive_read_reset_passphrase(a);

	/* File entries are sorted by the header offset, we should mostly
	 * use __archive_read_consume to advance a read point to avoid redundant
	 * data reading.  */
	offset = archive_filter_bytes(&a->archive, 0);
	if (offset < zip->entry->local_header_offset)
		__archive_read_consume(a,
		    zip->entry->local_header_offset - offset);
	else if (offset != zip->entry->local_header_offset) {
		__archive_read_seek(a, zip->entry->local_header_offset,
		    SEEK_SET);
	}
	zip->unconsumed = 0;
	r = zip_read_local_file_header(a, entry, zip);
	if (r != ARCHIVE_OK)
		return r;
	if (rsrc) {
		int ret2 = zip_read_mac_metadata(a, entry, rsrc);
		if (ret2 < ret)
			ret = ret2;
	}
	return (ret);
}
