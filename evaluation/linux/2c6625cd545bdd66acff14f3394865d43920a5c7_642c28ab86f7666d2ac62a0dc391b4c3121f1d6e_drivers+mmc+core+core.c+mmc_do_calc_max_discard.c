static unsigned int mmc_do_calc_max_discard(struct mmc_card *card,
					    unsigned int arg)
{
	struct mmc_host *host = card->host;
	unsigned int max_discard, x, y, qty = 0, max_qty, timeout;
	unsigned int last_timeout = 0;

	if (card->erase_shift)
		max_qty = UINT_MAX >> card->erase_shift;
	else if (mmc_card_sd(card))
		max_qty = UINT_MAX;
	else
		max_qty = UINT_MAX / card->erase_size;

	/* Find the largest qty with an OK timeout */
	do {
		y = 0;
		for (x = 1; x && x <= max_qty && max_qty - x >= qty; x <<= 1) {
			timeout = mmc_erase_timeout(card, arg, qty + x);
			if (timeout > host->max_busy_timeout)
				break;
			if (timeout < last_timeout)
				break;
			last_timeout = timeout;
			y = x;
		}
		qty += y;
	} while (y);

	if (!qty)
		return 0;

	/*
	 * When specifying a sector range to trim, chances are we might cross
	 * an erase-group boundary even if the amount of sectors is less than
	 * one erase-group.
	 * If we can only fit one erase-group in the controller timeout budget,
	 * we have to care that erase-group boundaries are not crossed by a
	 * single trim operation. We flag that special case with "eg_boundary".
	 * In all other cases we can just decrement qty and pretend that we
	 * always touch (qty + 1) erase-groups as a simple optimization.
	 */
	if (qty == 1)
		card->eg_boundary = 1;
	else
		qty--;

	/* Convert qty to sectors */
	if (card->erase_shift)
		max_discard = qty << card->erase_shift;
	else if (mmc_card_sd(card))
		max_discard = qty + 1;
	else
		max_discard = qty * card->erase_size;

	return max_discard;
}
