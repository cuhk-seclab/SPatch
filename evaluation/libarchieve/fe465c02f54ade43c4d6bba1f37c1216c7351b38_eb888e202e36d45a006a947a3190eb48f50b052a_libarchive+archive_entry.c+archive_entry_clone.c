struct archive_entry *
archive_entry_clone(struct archive_entry *entry)
{
	struct archive_entry *entry2;
	struct ae_xattr *xp;
	struct ae_sparse *sp;
	size_t s;
	const void *p;

	/* Allocate new structure and copy over all of the fields. */
	/* TODO: Should we copy the archive over?  Or require a new archive
	 * as an argument? */
	entry2 = archive_entry_new2(entry->archive);
	if (entry2 == NULL)
		return (NULL);
	entry2->ae_stat = entry->ae_stat;
	entry2->ae_fflags_set = entry->ae_fflags_set;
	entry2->ae_fflags_clear = entry->ae_fflags_clear;

	/* TODO: XXX If clone can have a different archive, what do we do here if
	 * character sets are different? XXX */
	archive_mstring_copy(&entry2->ae_fflags_text, &entry->ae_fflags_text);
	archive_mstring_copy(&entry2->ae_gname, &entry->ae_gname);
	archive_mstring_copy(&entry2->ae_hardlink, &entry->ae_hardlink);
	archive_mstring_copy(&entry2->ae_pathname, &entry->ae_pathname);
	archive_mstring_copy(&entry2->ae_sourcepath, &entry->ae_sourcepath);
	archive_mstring_copy(&entry2->ae_symlink, &entry->ae_symlink);
	entry2->ae_set = entry->ae_set;
	archive_mstring_copy(&entry2->ae_uname, &entry->ae_uname);

	/* Copy symlink type */
	entry2->ae_symlink_type = entry->ae_symlink_type;

	/* Copy encryption status */
	entry2->encryption = entry->encryption;

	/* Copy digests */
#define copy_digest(_e2, _e, _t) \
	memcpy(_e2->digest._t, _e->digest._t, sizeof(_e2->digest._t))

	copy_digest(entry2, entry, md5);
	copy_digest(entry2, entry, rmd160);
	copy_digest(entry2, entry, sha1);
	copy_digest(entry2, entry, sha256);
	copy_digest(entry2, entry, sha384);
	copy_digest(entry2, entry, sha512);

#undef copy_digest
	
	/* Copy ACL data over. */
	archive_acl_copy(&entry2->acl, &entry->acl);

	/* Copy Mac OS metadata. */
	p = archive_entry_mac_metadata(entry, &s);
	archive_entry_copy_mac_metadata(entry2, p, s);

	/* Copy xattr data over. */
	xp = entry->xattr_head;
	while (xp != NULL) {
		archive_entry_xattr_add_entry(entry2,
		    xp->name, xp->value, xp->size);
		xp = xp->next;
	}

	/* Copy sparse data over. */
	sp = entry->sparse_head;
	while (sp != NULL) {
		archive_entry_sparse_add_entry(entry2,
		    sp->offset, sp->length);
		sp = sp->next;
	}

	return (entry2);
}
