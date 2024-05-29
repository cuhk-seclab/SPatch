static void *
find_inlist_lock(struct list_head *head, const char *name, const char *prefix,
		 int *error, struct mutex *mutex)
{
	return try_then_request_module(
			find_inlist_lock_noload(head, name, error, mutex),
			"%s%s", prefix, name);
}
