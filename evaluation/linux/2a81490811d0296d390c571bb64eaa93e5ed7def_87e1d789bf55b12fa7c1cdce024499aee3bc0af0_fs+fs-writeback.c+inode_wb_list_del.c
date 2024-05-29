void inode_wb_list_del(struct inode *inode)
{
	struct bdi_writeback *wb;

	wb = inode_to_wb_and_lock_list(inode);
	inode_wb_list_del_locked(inode, wb);
	spin_unlock(&wb->list_lock);
}
