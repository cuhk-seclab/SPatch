static void
insert_passphrase_to_head(struct archive_read *a,
    struct archive_read_passphrase *p)
{
	p->next = a->passphrases.first;
	a->passphrases.first = p;
	if (&a->passphrases.first == a->passphrases.last) {
		a->passphrases.last = &p->next;
		p->next = NULL;
	}
}
