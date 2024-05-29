static struct ext4_prealloc_space *
ext4_mb_check_group_pa(ext4_fsblk_t goal_block,
			struct ext4_prealloc_space *pa,
			struct ext4_prealloc_space *cpa)
{
	ext4_fsblk_t cur_distance, new_distance;

	if (cpa == NULL) {
		atomic_inc(&pa->pa_count);
		return pa;
	}
	cur_distance = abs64(goal_block - cpa->pa_pstart);
	new_distance = abs64(goal_block - pa->pa_pstart);

	if (cur_distance <= new_distance)
		return cpa;

	/* drop the previous reference */
	atomic_dec(&cpa->pa_count);
	atomic_inc(&pa->pa_count);
	return pa;
}
