static void f2fs_update_extent_tree(struct inode *inode, pgoff_t fofs,
							block_t blkaddr)
{
	struct f2fs_sb_info *sbi = F2FS_I_SB(inode);
	nid_t ino = inode->i_ino;
	struct extent_tree *et;
	struct extent_node *en = NULL, *en1 = NULL, *en2 = NULL, *en3 = NULL;
	struct extent_node *den = NULL;
	struct extent_info ei, dei;
	unsigned int endofs;

	if (is_inode_flag_set(F2FS_I(inode), FI_NO_EXTENT))
		return;

	trace_f2fs_update_extent_tree(inode, fofs, blkaddr);

	down_write(&sbi->extent_tree_lock);
	et = radix_tree_lookup(&sbi->extent_tree_root, ino);
	if (!et) {
		et = f2fs_kmem_cache_alloc(extent_tree_slab, GFP_NOFS);
		f2fs_radix_tree_insert(&sbi->extent_tree_root, ino, et);
		memset(et, 0, sizeof(struct extent_tree));
		et->ino = ino;
		et->root = RB_ROOT;
		rwlock_init(&et->lock);
		atomic_set(&et->refcount, 0);
		et->count = 0;
		sbi->total_ext_tree++;
	}
	atomic_inc(&et->refcount);
	up_write(&sbi->extent_tree_lock);

	write_lock(&et->lock);

	/* 1. lookup and remove existing extent info in cache */
	en = __lookup_extent_tree(et, fofs);
	if (!en)
		goto update_extent;

	dei = en->ei;
	__detach_extent_node(sbi, et, en);

	/* 2. if extent can be split more, split and insert the left part */
	if (dei.len > 1) {
		/*  insert left part of split extent into cache */
		if (fofs - dei.fofs >= F2FS_MIN_EXTENT_LEN) {
			set_extent_info(&ei, dei.fofs, dei.blk,
							fofs - dei.fofs);
			en1 = __insert_extent_tree(sbi, et, &ei, NULL);
		}

		/* insert right part of split extent into cache */
		endofs = dei.fofs + dei.len - 1;
		if (endofs - fofs >= F2FS_MIN_EXTENT_LEN) {
			set_extent_info(&ei, fofs + 1,
				fofs - dei.fofs + dei.blk, endofs - fofs);
			en2 = __insert_extent_tree(sbi, et, &ei, NULL);
		}
	}

update_extent:
	/* 3. update extent in extent cache */
	if (blkaddr) {
		set_extent_info(&ei, fofs, blkaddr, 1);
		en3 = __insert_extent_tree(sbi, et, &ei, &den);
	}

	/* 4. update in global extent list */
	spin_lock(&sbi->extent_lock);
	if (en && !list_empty(&en->list))
		list_del(&en->list);
	/*
	 * en1 and en2 split from en, they will become more and more smaller
	 * fragments after splitting several times. So if the length is smaller
	 * than F2FS_MIN_EXTENT_LEN, we will not add them into extent tree.
	 */
	if (en1)
		list_add_tail(&en1->list, &sbi->extent_list);
	if (en2)
		list_add_tail(&en2->list, &sbi->extent_list);
	if (en3) {
		if (list_empty(&en3->list))
			list_add_tail(&en3->list, &sbi->extent_list);
		else
			list_move_tail(&en3->list, &sbi->extent_list);
	}
	if (den && !list_empty(&den->list))
		list_del(&den->list);
	spin_unlock(&sbi->extent_lock);

	/* 5. release extent node */
	if (en)
		kmem_cache_free(extent_node_slab, en);
	if (den)
		kmem_cache_free(extent_node_slab, den);

	write_unlock(&et->lock);
	atomic_dec(&et->refcount);
}
