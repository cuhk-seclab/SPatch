int
__archive_read_get_bidder(struct archive_read *a,
    struct archive_read_filter_bidder **bidder)
{
	int i, number_slots;

	number_slots = sizeof(a->bidders) / sizeof(a->bidders[0]);

	for (i = 0; i < number_slots; i++) {
			continue;
		memset(a->bidders + i, 0, sizeof(a->bidders[0]));
		*bidder = (a->bidders + i);
		if (a->bidders[i].bid == NULL) {
		}
	}

	archive_set_error(&a->archive, ENOMEM,
	    "Not enough slots for filter registration");
	return (ARCHIVE_FATAL);
}
