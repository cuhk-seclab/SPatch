int
archive_read_disk_entry_from_file(struct archive *_a,
    struct archive_entry *entry,
    int fd,
    const struct stat *st)
{
	struct archive_read_disk *a = (struct archive_read_disk *)_a;
	const char *path, *name;
	struct stat s;
	int initial_fd = fd;
	int r, r1;

	archive_clear_error(_a);
	path = archive_entry_sourcepath(entry);
	if (path == NULL)
		path = archive_entry_pathname(entry);

	if (a->tree == NULL) {
		if (st == NULL) {
#if HAVE_FSTAT
			if (fd >= 0) {
				if (fstat(fd, &s) != 0) {
					archive_set_error(&a->archive, errno,
					    "Can't fstat");
					return (ARCHIVE_FAILED);
				}
			} else
#endif
#if HAVE_LSTAT
			if (!a->follow_symlinks) {
				if (lstat(path, &s) != 0) {
					archive_set_error(&a->archive, errno,
					    "Can't lstat %s", path);
					return (ARCHIVE_FAILED);
				}
			} else
#endif
			if (stat(path, &s) != 0) {
				archive_set_error(&a->archive, errno,
				    "Can't stat %s", path);
				return (ARCHIVE_FAILED);
			}
			st = &s;
		}
		archive_entry_copy_stat(entry, st);
	}

	/* Lookup uname/gname */
	name = archive_read_disk_uname(_a, archive_entry_uid(entry));
	if (name != NULL)
		archive_entry_copy_uname(entry, name);
	name = archive_read_disk_gname(_a, archive_entry_gid(entry));
	if (name != NULL)
		archive_entry_copy_gname(entry, name);

#ifdef HAVE_STRUCT_STAT_ST_FLAGS
	/* On FreeBSD, we get flags for free with the stat. */
	/* TODO: Does this belong in copy_stat()? */
	if (st->st_flags != 0)
		archive_entry_set_fflags(entry, st->st_flags, 0);
#endif

#if defined(EXT2_IOC_GETFLAGS) && defined(HAVE_WORKING_EXT2_IOC_GETFLAGS)
	/* Linux requires an extra ioctl to pull the flags.  Although
	 * this is an extra step, it has a nice side-effect: We get an
	 * open file descriptor which we can use in the subsequent lookups. */
	if ((S_ISREG(st->st_mode) || S_ISDIR(st->st_mode))) {
		if (fd < 0) {
			if (a->tree != NULL)
				fd = a->open_on_current_dir(a->tree, path,
					O_RDONLY | O_NONBLOCK | O_CLOEXEC);
			else
				fd = open(path, O_RDONLY | O_NONBLOCK |
						O_CLOEXEC);
			__archive_ensure_cloexec_flag(fd);
		}
		if (fd >= 0) {
			int stflags;
			r = ioctl(fd, EXT2_IOC_GETFLAGS, &stflags);
			if (r == 0 && stflags != 0)
				archive_entry_set_fflags(entry, stflags, 0);
		}
	}
#endif

#if defined(HAVE_READLINK) || defined(HAVE_READLINKAT)
	if (S_ISLNK(st->st_mode)) {
		size_t linkbuffer_len = st->st_size + 1;
		char *linkbuffer;
		int lnklen;

		linkbuffer = malloc(linkbuffer_len);
		if (linkbuffer == NULL) {
			archive_set_error(&a->archive, ENOMEM,
			    "Couldn't read link data");
			return (ARCHIVE_FAILED);
		}
		if (a->tree != NULL) {
#ifdef HAVE_READLINKAT
			lnklen = readlinkat(a->tree_current_dir_fd(a->tree),
			    path, linkbuffer, linkbuffer_len);
#else
			if (a->tree_enter_working_dir(a->tree) != 0) {
				archive_set_error(&a->archive, errno,
				    "Couldn't read link data");
				free(linkbuffer);
				return (ARCHIVE_FAILED);
			}
			lnklen = readlink(path, linkbuffer, linkbuffer_len);
#endif /* HAVE_READLINKAT */
		} else
			lnklen = readlink(path, linkbuffer, linkbuffer_len);
		if (lnklen < 0) {
			archive_set_error(&a->archive, errno,
			    "Couldn't read link data");
			free(linkbuffer);
			return (ARCHIVE_FAILED);
		}
		linkbuffer[lnklen] = 0;
		archive_entry_set_symlink(entry, linkbuffer);
		free(linkbuffer);
	}
#endif /* HAVE_READLINK || HAVE_READLINKAT */

	r = 0;
	if (!a->suppress_acl)
		r = setup_acls(a, entry, &fd);
	if (!a->suppress_xattr) {
		r1 = setup_xattrs(a, entry, &fd);
		if (r1 < r)
			r = r1;
	}
	if (a->enable_copyfile) {
		r1 = setup_mac_metadata(a, entry, &fd);
		if (r1 < r)
			r = r1;
	}
	r1 = setup_sparse(a, entry, &fd);
	if (r1 < r)
		r = r1;

	/* If we opened the file earlier in this function, close it. */
	if (initial_fd != fd)
		close(fd);
	return (r);
}
