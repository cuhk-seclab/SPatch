struct btrfs_qgroup_extent_record
*btrfs_qgroup_insert_dirty_extent(struct btrfs_delayed_ref_root *delayed_refs,
				  struct btrfs_qgroup_extent_record *record)
{
	struct rb_node **p = &delayed_refs->dirty_extent_root.rb_node;
	struct rb_node *parent_node = NULL;
	struct btrfs_qgroup_extent_record *entry;
	u64 bytenr = record->bytenr;

	assert_spin_locked(&delayed_refs->lock);

	while (*p) {
		parent_node = *p;
		entry = rb_entry(parent_node, struct btrfs_qgroup_extent_record,
				 node);
		if (bytenr < entry->bytenr)
			p = &(*p)->rb_left;
		else if (bytenr > entry->bytenr)
			p = &(*p)->rb_right;
		else
			return entry;
	}

	rb_link_node(&record->node, parent_node, p);
	rb_insert_color(&record->node, &delayed_refs->dirty_extent_root);
	return NULL;
}
