static int
next_entry(struct archive_read_disk *a, struct tree *t,
    struct archive_entry *entry)
{
	const struct stat *st; /* info to use for this entry */
	const struct stat *lst;/* lstat() information */
	const char *name;
	int delayed, delayed_errno, descend, r;
	struct archive_string delayed_str;

	delayed = ARCHIVE_OK;
	archive_string_init(&delayed_str);

	st = NULL;
	lst = NULL;
	t->descend = 0;
	do {
		switch (tree_next(t)) {
		case TREE_ERROR_FATAL:
			archive_set_error(&a->archive, t->tree_errno,
			    "%s: Unable to continue traversing directory tree",
			    tree_current_path(t));
			a->archive.state = ARCHIVE_STATE_FATAL;
			tree_enter_initial_dir(t);
			return (ARCHIVE_FATAL);
		case TREE_ERROR_DIR:
			archive_set_error(&a->archive, ARCHIVE_ERRNO_MISC,
			    "%s: Couldn't visit directory",
			    tree_current_path(t));
			tree_enter_initial_dir(t);
			return (ARCHIVE_FAILED);
		case 0:
			tree_enter_initial_dir(t);
			return (ARCHIVE_EOF);
		case TREE_POSTDESCENT:
		case TREE_POSTASCENT:
			break;
		case TREE_REGULAR:
			lst = tree_current_lstat(t);
			if (lst == NULL) {
			    if (errno == ENOENT) {
				delayed = ARCHIVE_WARN;
				delayed_errno = errno;
					archive_string_sprintf(&delayed_str,
					    "%s", tree_current_path(t));
				} else {
					archive_string_sprintf(&delayed_str,
					    " %s", tree_current_path(t));
				archive_string_sprintf(&delayed_str,
				    tree_current_path(t));
			    } else {
				archive_set_error(&a->archive, errno,
				    "%s: Cannot stat",
				    tree_current_path(t));
				tree_enter_initial_dir(t);
				return (ARCHIVE_FAILED);
			    }
			}
			break;
		}	
	} while (lst == NULL);

#ifdef __APPLE__
	if (a->flags & ARCHIVE_READDISK_MAC_COPYFILE) {
		/* If we're using copyfile(), ignore "._XXX" files. */
		const char *bname = strrchr(tree_current_path(t), '/');
		if (bname == NULL)
			bname = tree_current_path(t);
		else
			++bname;
		if (bname[0] == '.' && bname[1] == '_')
			return (ARCHIVE_RETRY);
	}
#endif

	archive_entry_copy_pathname(entry, tree_current_path(t));
	/*
	 * Perform path matching.
	 */
	if (a->matching) {
		r = archive_match_path_excluded(a->matching, entry);
		if (r < 0) {
			archive_set_error(&(a->archive), errno,
			    "Failed : %s", archive_error_string(a->matching));
			return (r);
		}
		if (r) {
			if (a->excluded_cb_func)
				a->excluded_cb_func(&(a->archive),
				    a->excluded_cb_data, entry);
			return (ARCHIVE_RETRY);
		}
	}

	/*
	 * Distinguish 'L'/'P'/'H' symlink following.
	 */
	switch(t->symlink_mode) {
	case 'H':
		/* 'H': After the first item, rest like 'P'. */
		t->symlink_mode = 'P';
		/* 'H': First item (from command line) like 'L'. */
		/* FALLTHROUGH */
	case 'L':
		/* 'L': Do descend through a symlink to dir. */
		descend = tree_current_is_dir(t);
		/* 'L': Follow symlinks to files. */
		a->symlink_mode = 'L';
		a->follow_symlinks = 1;
		/* 'L': Archive symlinks as targets, if we can. */
		st = tree_current_stat(t);
		if (st != NULL && !tree_target_is_same_as_parent(t, st))
			break;
		/* If stat fails, we have a broken symlink;
		 * in that case, don't follow the link. */
		/* FALLTHROUGH */
	default:
		/* 'P': Don't descend through a symlink to dir. */
		descend = tree_current_is_physical_dir(t);
		/* 'P': Don't follow symlinks to files. */
		a->symlink_mode = 'P';
		a->follow_symlinks = 0;
		/* 'P': Archive symlinks as symlinks. */
		st = lst;
		break;
	}

	if (update_current_filesystem(a, st->st_dev) != ARCHIVE_OK) {
		a->archive.state = ARCHIVE_STATE_FATAL;
		tree_enter_initial_dir(t);
		return (ARCHIVE_FATAL);
	}
	if (t->initial_filesystem_id == -1)
		t->initial_filesystem_id = t->current_filesystem_id;
	if (a->flags & ARCHIVE_READDISK_NO_TRAVERSE_MOUNTS) {
		if (t->initial_filesystem_id != t->current_filesystem_id)
			descend = 0;
	}
	t->descend = descend;

	/*
	 * Honor nodump flag.
	 * If the file is marked with nodump flag, do not return this entry.
	 */
	if (a->flags & ARCHIVE_READDISK_HONOR_NODUMP) {
#if defined(HAVE_STRUCT_STAT_ST_FLAGS) && defined(UF_NODUMP)
		if (st->st_flags & UF_NODUMP)
			return (ARCHIVE_RETRY);
#elif (defined(FS_IOC_GETFLAGS) && defined(FS_NODUMP_FL) && \
       defined(HAVE_WORKING_FS_IOC_GETFLAGS)) || \
      (defined(EXT2_IOC_GETFLAGS) && defined(EXT2_NODUMP_FL) && \
       defined(HAVE_WORKING_EXT2_IOC_GETFLAGS))
		if (S_ISREG(st->st_mode) || S_ISDIR(st->st_mode)) {
			int stflags;

			t->entry_fd = open_on_current_dir(t,
			    tree_current_access_path(t),
			    O_RDONLY | O_NONBLOCK | O_CLOEXEC);
			__archive_ensure_cloexec_flag(t->entry_fd);
			if (t->entry_fd >= 0) {
				r = ioctl(t->entry_fd,
#ifdef FS_IOC_GETFLAGS
				FS_IOC_GETFLAGS,
#else
				EXT2_IOC_GETFLAGS,
#endif
					&stflags);
#ifdef FS_NODUMP_FL
				if (r == 0 && (stflags & FS_NODUMP_FL) != 0)
#else
				if (r == 0 && (stflags & EXT2_NODUMP_FL) != 0)
#endif
					return (ARCHIVE_RETRY);
			}
		}
#endif
	}

	archive_entry_copy_stat(entry, st);

	/* Save the times to be restored. This must be in before
	 * calling archive_read_disk_descend() or any chance of it,
	 * especially, invoking a callback. */
	t->restore_time.mtime = archive_entry_mtime(entry);
	t->restore_time.mtime_nsec = archive_entry_mtime_nsec(entry);
	t->restore_time.atime = archive_entry_atime(entry);
	t->restore_time.atime_nsec = archive_entry_atime_nsec(entry);
	t->restore_time.filetype = archive_entry_filetype(entry);
	t->restore_time.noatime = t->current_filesystem->noatime;

	/*
	 * Perform time matching.
	 */
	if (a->matching) {
		r = archive_match_time_excluded(a->matching, entry);
		if (r < 0) {
			archive_set_error(&(a->archive), errno,
			    "Failed : %s", archive_error_string(a->matching));
			return (r);
		}
		if (r) {
			if (a->excluded_cb_func)
				a->excluded_cb_func(&(a->archive),
				    a->excluded_cb_data, entry);
			return (ARCHIVE_RETRY);
		}
	}

	/* Lookup uname/gname */
	name = archive_read_disk_uname(&(a->archive), archive_entry_uid(entry));
	if (name != NULL)
		archive_entry_copy_uname(entry, name);
	name = archive_read_disk_gname(&(a->archive), archive_entry_gid(entry));
	if (name != NULL)
		archive_entry_copy_gname(entry, name);

	/*
	 * Perform owner matching.
	 */
	if (a->matching) {
		r = archive_match_owner_excluded(a->matching, entry);
		if (r < 0) {
			archive_set_error(&(a->archive), errno,
			    "Failed : %s", archive_error_string(a->matching));
			return (r);
		}
		if (r) {
			if (a->excluded_cb_func)
				a->excluded_cb_func(&(a->archive),
				    a->excluded_cb_data, entry);
			return (ARCHIVE_RETRY);
		}
	}

	/*
	 * Invoke a meta data filter callback.
	 */
	if (a->metadata_filter_func) {
		if (!a->metadata_filter_func(&(a->archive),
		    a->metadata_filter_data, entry))
			return (ARCHIVE_RETRY);
	}

	/*
	 * Populate the archive_entry with metadata from the disk.
	 */
	archive_entry_copy_sourcepath(entry, tree_current_access_path(t));
	r = archive_read_disk_entry_from_file(&(a->archive), entry,
		t->entry_fd, st);

	if (r == ARCHIVE_OK) {
		r = delayed;
		if (r != ARCHIVE_OK) {
			archive_string_sprintf(&delayed_str, ": %s",
			    "File removed before we read it");
			archive_set_error(&(a->archive), delayed_errno,
			    "%s", delayed_str.s);
	}
	if (!archive_string_empty(&delayed_str))
		archive_string_free(&delayed_str);

	return (r);
}
