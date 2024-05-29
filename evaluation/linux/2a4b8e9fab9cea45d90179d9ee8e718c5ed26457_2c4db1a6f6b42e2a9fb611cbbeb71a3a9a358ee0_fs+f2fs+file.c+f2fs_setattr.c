int f2fs_setattr(struct dentry *dentry, struct iattr *attr)
{
	struct inode *inode = d_inode(dentry);
	struct f2fs_inode_info *fi = F2FS_I(inode);
	int err;

	err = inode_change_ok(inode, attr);
	if (err)
		return err;

	if (attr->ia_valid & ATTR_SIZE) {
		if (f2fs_encrypted_inode(inode) &&
				f2fs_get_encryption_info(inode))
			return -EACCES;

		if (attr->ia_size <= i_size_read(inode)) {
			truncate_setsize(inode, attr->ia_size);
			err = f2fs_truncate(inode, true);
			if (err)
				return err;
			f2fs_balance_fs(F2FS_I_SB(inode), true);
		} else {
			/*
			 * do not trim all blocks after i_size if target size is
			 * larger than i_size.
			 */
			truncate_setsize(inode, attr->ia_size);

			/* should convert inline inode here */
			if (!f2fs_may_inline_data(inode)) {
				err = f2fs_convert_inline_inode(inode);
				if (err)
					return err;
			}
			inode->i_mtime = inode->i_ctime = CURRENT_TIME;
		}
	}

	__setattr_copy(inode, attr);

	if (attr->ia_valid & ATTR_MODE) {
		err = posix_acl_chmod(inode, get_inode_mode(inode));
		if (err || is_inode_flag_set(fi, FI_ACL_MODE)) {
			inode->i_mode = fi->i_acl_mode;
			clear_inode_flag(fi, FI_ACL_MODE);
		}
	}

	mark_inode_dirty(inode);
	return err;
}
