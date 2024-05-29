int
archive_read_disk_descend(struct archive *_a)
{
	struct archive_read_disk *a = (struct archive_read_disk *)_a;
	struct tree *t = a->tree;

	archive_check_magic(_a, ARCHIVE_READ_DISK_MAGIC,
	    ARCHIVE_STATE_HEADER | ARCHIVE_STATE_DATA,
	    "archive_read_disk_descend");

	if (t->visit_type != TREE_REGULAR || !t->descend)
		return (ARCHIVE_OK);

	/*
	 * We must not treat the initial specified path as a physical dir,
	 * because if we do then we will try and ascend out of it by opening
	 * ".." which is (a) wrong and (b) causes spurious permissions errors
	 * if ".." is not readable by us. Instead, treat it as if it were a
	 * symlink. (This uses an extra fd, but it can only happen once at the
	 * top level of a traverse.) But we can't necessarily assume t->st is
	 * valid here (though t->lst is), which complicates the logic a
	 * little.
	 */
	if (tree_current_is_physical_dir(t)) {
		tree_push(t, t->basename, t->current_filesystem_id,
		    t->lst.st_dev, t->lst.st_ino, &t->restore_time);
		if (t->stack->parent->parent != NULL)
			t->stack->flags |= isDir;
		else
			t->stack->flags |= isDirLink;
	} else if (tree_current_is_dir(t)) {
		tree_push(t, t->basename, t->current_filesystem_id,
		    t->st.st_dev, t->st.st_ino, &t->restore_time);
		t->stack->flags |= isDirLink;
	}
	t->descend = 0;
	return (ARCHIVE_OK);
}
