static int nfs_check_inode_attributes(struct inode *inode, struct nfs_fattr *fattr)
{
	struct nfs_inode *nfsi = NFS_I(inode);
	loff_t cur_size, new_isize;
	unsigned long invalid = 0;


	if (nfs_have_delegated_attributes(inode))
		return 0;
	/* Has the inode gone and changed behind our back? */
	if ((fattr->valid & NFS_ATTR_FATTR_FILEID) && nfsi->fileid != fattr->fileid)
		return -EIO;
	if ((fattr->valid & NFS_ATTR_FATTR_TYPE) && (inode->i_mode & S_IFMT) != (fattr->mode & S_IFMT))
		return -EIO;

	if ((fattr->valid & NFS_ATTR_FATTR_CHANGE) != 0 &&
			inode->i_version != fattr->change_attr)
		invalid |= NFS_INO_INVALID_ATTR|NFS_INO_REVAL_PAGECACHE;

	/* Verify a few of the more important attributes */
	if ((fattr->valid & NFS_ATTR_FATTR_MTIME) && !timespec_equal(&inode->i_mtime, &fattr->mtime))
		invalid |= NFS_INO_INVALID_ATTR;

	if (fattr->valid & NFS_ATTR_FATTR_SIZE) {
		cur_size = i_size_read(inode);
		new_isize = nfs_size_to_loff_t(fattr->size);
		if (cur_size != new_isize)
			invalid |= NFS_INO_INVALID_ATTR|NFS_INO_REVAL_PAGECACHE;
	}
	if (nfsi->nrequests != 0)
		invalid &= ~NFS_INO_REVAL_PAGECACHE;

	/* Have any file permissions changed? */
	if ((fattr->valid & NFS_ATTR_FATTR_MODE) && (inode->i_mode & S_IALLUGO) != (fattr->mode & S_IALLUGO))
		invalid |= NFS_INO_INVALID_ATTR | NFS_INO_INVALID_ACCESS | NFS_INO_INVALID_ACL;
	if ((fattr->valid & NFS_ATTR_FATTR_OWNER) && !uid_eq(inode->i_uid, fattr->uid))
		invalid |= NFS_INO_INVALID_ATTR | NFS_INO_INVALID_ACCESS | NFS_INO_INVALID_ACL;
	if ((fattr->valid & NFS_ATTR_FATTR_GROUP) && !gid_eq(inode->i_gid, fattr->gid))
		invalid |= NFS_INO_INVALID_ATTR | NFS_INO_INVALID_ACCESS | NFS_INO_INVALID_ACL;

	/* Has the link count changed? */
	if ((fattr->valid & NFS_ATTR_FATTR_NLINK) && inode->i_nlink != fattr->nlink)
		invalid |= NFS_INO_INVALID_ATTR;

	if ((fattr->valid & NFS_ATTR_FATTR_ATIME) && !timespec_equal(&inode->i_atime, &fattr->atime))
		invalid |= NFS_INO_INVALID_ATIME;

	if (invalid != 0)
		nfs_set_cache_invalid(inode, invalid);

	nfsi->read_cache_jiffies = fattr->time_start;
	return 0;
}
