void libcfs_debug_dumplog_internal(void *arg)
{
	void *journal_info;

	journal_info = current->journal_info;
	current->journal_info = NULL;

	if (strncmp(libcfs_debug_file_path_arr, "NONE", 4) != 0) {
		snprintf(debug_file_name, sizeof(debug_file_name) - 1,
			 "%s.%lld.%ld", libcfs_debug_file_path_arr,
			 (s64)ktime_get_real_seconds(), (long_ptr_t)arg);
		pr_alert("LustreError: dumping log to %s\n", debug_file_name);
		cfs_tracefile_dump_all_pages(debug_file_name);
		libcfs_run_debug_log_upcall(debug_file_name);
	}

	current->journal_info = journal_info;
}
