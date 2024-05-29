static int rar5_bid(struct archive_read* a, int best_bid) {
	int my_bid;

	if(best_bid > 30)
		return -1;

	my_bid = bid_standard(a);
	if(my_bid > -1) {
		return my_bid;
	}
	my_bid = bid_sfx(a);
	if (my_bid > -1) {
		return my_bid;
	}

	return -1;
}
