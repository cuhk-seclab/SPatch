void
archive_acl_clear(struct archive_acl *acl)
{
	struct archive_acl_entry *ap;

	while (acl->acl_head != NULL) {
		ap = acl->acl_head->next;
		archive_mstring_clean(&acl->acl_head->name);
		free(acl->acl_head);
		acl->acl_head = ap;
	}
	free(acl->acl_text_w);
	acl->acl_text_w = NULL;
	free(acl->acl_text);
	acl->acl_text = NULL;
	acl->acl_p = NULL;
	acl->acl_types = 0;
	acl->acl_state = 0; /* Not counting. */
}
