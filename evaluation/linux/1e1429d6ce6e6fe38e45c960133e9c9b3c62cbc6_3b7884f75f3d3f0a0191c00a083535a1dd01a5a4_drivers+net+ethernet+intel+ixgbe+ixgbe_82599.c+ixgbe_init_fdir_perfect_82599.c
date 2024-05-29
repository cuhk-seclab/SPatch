s32 ixgbe_init_fdir_perfect_82599(struct ixgbe_hw *hw, u32 fdirctrl)
{
	/*
	 * Continue setup of fdirctrl register bits:
	 *  Turn perfect match filtering on
	 *  Initialize the drop queue
	 *  Move the flexible bytes to use the ethertype - shift 6 words
	 *  Set the maximum length per hash bucket to 0xA filters
	 *  Send interrupt when 64 (0x4 * 16) filters are left
	 */
	fdirctrl |= IXGBE_FDIRCTRL_PERFECT_MATCH |
		    (IXGBE_FDIR_DROP_QUEUE << IXGBE_FDIRCTRL_DROP_Q_SHIFT) |
		    (0x6 << IXGBE_FDIRCTRL_FLEX_SHIFT) |
		    (0xA << IXGBE_FDIRCTRL_MAX_LENGTH_SHIFT) |
		    (4 << IXGBE_FDIRCTRL_FULL_THRESH_SHIFT);

	/* write hashes and fdirctrl register, poll for completion */
	ixgbe_fdir_enable_82599(hw, fdirctrl);

	return 0;
}
