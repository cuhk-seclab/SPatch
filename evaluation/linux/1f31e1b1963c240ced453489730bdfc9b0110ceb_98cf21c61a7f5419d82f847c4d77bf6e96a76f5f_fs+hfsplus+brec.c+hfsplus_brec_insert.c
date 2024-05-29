int hfs_brec_insert(struct hfs_find_data *fd, void *entry, int entry_len)
{
	struct hfs_btree *tree;
	struct hfs_bnode *node, *new_node;
	int size, key_len, rec;
	int data_off, end_off;
	int idx_rec_off, data_rec_off, end_rec_off;
	__be32 cnid;

	tree = fd->tree;
	if (!fd->bnode) {
		if (!tree->root)
			hfs_btree_inc_height(tree);
		fd->bnode = hfs_bnode_find(tree, tree->leaf_head);
		if (IS_ERR(fd->bnode))
			return PTR_ERR(fd->bnode);
		fd->record = -1;
	}
	new_node = NULL;
	key_len = be16_to_cpu(fd->search_key->key_len) + 2;
again:
	/* new record idx and complete record size */
	rec = fd->record + 1;
	size = key_len + entry_len;

	node = fd->bnode;
	hfs_bnode_dump(node);
	/* get last offset */
	end_rec_off = tree->node_size - (node->num_recs + 1) * 2;
	end_off = hfs_bnode_read_u16(node, end_rec_off);
	end_rec_off -= 2;
	hfs_dbg(BNODE_MOD, "insert_rec: %d, %d, %d, %d\n",
		rec, size, end_off, end_rec_off);
	if (size > end_rec_off - end_off) {
		if (new_node)
			panic("not enough room!\n");
		new_node = hfs_bnode_split(fd);
		if (IS_ERR(new_node))
			return PTR_ERR(new_node);
		goto again;
	}
	if (node->type == HFS_NODE_LEAF) {
		tree->leaf_count++;
		mark_inode_dirty(tree->inode);
	}
	node->num_recs++;
	/* write new last offset */
	hfs_bnode_write_u16(node,
		offsetof(struct hfs_bnode_desc, num_recs),
		node->num_recs);
	hfs_bnode_write_u16(node, end_rec_off, end_off + size);
	data_off = end_off;
	data_rec_off = end_rec_off + 2;
	idx_rec_off = tree->node_size - (rec + 1) * 2;
	if (idx_rec_off == data_rec_off)
		goto skip;
	/* move all following entries */
	do {
		data_off = hfs_bnode_read_u16(node, data_rec_off + 2);
		hfs_bnode_write_u16(node, data_rec_off, data_off + size);
		data_rec_off += 2;
	} while (data_rec_off < idx_rec_off);

	/* move data away */
	hfs_bnode_move(node, data_off + size, data_off,
		       end_off - data_off);

skip:
	hfs_bnode_write(node, fd->search_key, data_off, key_len);
	hfs_bnode_write(node, entry, data_off + key_len, entry_len);
	hfs_bnode_dump(node);

	/*
	 * update parent key if we inserted a key
	 * at the start of the node and it is not the new node
	 */
	if (!rec && new_node != node) {
		hfs_bnode_read_key(node, fd->search_key, data_off + size);
		hfs_brec_update_parent(fd);
	}

	if (new_node) {
		hfs_bnode_put(fd->bnode);
		if (!new_node->parent) {
			hfs_btree_inc_height(tree);
			new_node->parent = tree->root;
		}
		fd->bnode = hfs_bnode_find(tree, new_node->parent);

		/* create index data entry */
		cnid = cpu_to_be32(new_node->this);
		entry = &cnid;
		entry_len = sizeof(cnid);

		/* get index key */
		hfs_bnode_read_key(new_node, fd->search_key, 14);
		__hfs_brec_find(fd->bnode, fd, hfs_find_rec_by_key);

		hfs_bnode_put(new_node);
		new_node = NULL;

		if ((tree->attributes & HFS_TREE_VARIDXKEYS) ||
				(tree->cnid == HFSPLUS_ATTR_CNID))
			key_len = be16_to_cpu(fd->search_key->key_len) + 2;
		else {
			fd->search_key->key_len =
				cpu_to_be16(tree->max_key_len);
			key_len = tree->max_key_len + 2;
		}
		goto again;
	}

	return 0;
}
