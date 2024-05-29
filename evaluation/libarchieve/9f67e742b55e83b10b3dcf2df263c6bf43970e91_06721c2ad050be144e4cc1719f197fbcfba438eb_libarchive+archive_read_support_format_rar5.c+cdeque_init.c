static int cdeque_init(struct cdeque* d, int max_capacity_power_of_2) {
	if(d == NULL || max_capacity_power_of_2 == 0)
		return CDE_PARAM;

	d->cap_mask = max_capacity_power_of_2 - 1;
	d->arr = NULL;

	if((max_capacity_power_of_2 & d->cap_mask) != 0)
		return CDE_PARAM;

	cdeque_clear(d);
	d->arr = malloc(sizeof(void*) * max_capacity_power_of_2);

	return d->arr ? CDE_OK : CDE_ALLOC;
}
