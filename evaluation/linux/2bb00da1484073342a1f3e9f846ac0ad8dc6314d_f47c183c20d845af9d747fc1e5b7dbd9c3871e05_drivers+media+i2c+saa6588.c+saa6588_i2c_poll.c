static void saa6588_i2c_poll(struct saa6588 *s)
{
	struct i2c_client *client = v4l2_get_subdevdata(&s->sd);
	unsigned long flags;
	unsigned char tmpbuf[6];
	unsigned char blocknum;
	unsigned char tmp;

	/* Although we only need 3 bytes, we have to read at least 6.
	   SAA6588 returns garbage otherwise. */
	if (6 != i2c_master_recv(client, &tmpbuf[0], 6)) {
		if (debug > 1)
			dprintk(PREFIX "read error!\n");
		return;
	}

	s->sync = tmpbuf[0] & 0x10;
	if (!s->sync)
		return;
	blocknum = tmpbuf[0] >> 5;
	if (blocknum == s->last_blocknum) {
		if (debug > 3)
			dprintk("Saw block %d again.\n", blocknum);
		return;
	}

	s->last_blocknum = blocknum;

	/*
	   Byte order according to v4l2 specification:

	   Byte 0: Least Significant Byte of RDS Block
	   Byte 1: Most Significant Byte of RDS Block
	   Byte 2 Bit 7: Error bit. Indicates that an uncorrectable error
	   occurred during reception of this block.
	   Bit 6: Corrected bit. Indicates that an error was
	   corrected for this data block.
	   Bits 5-3: Same as bits 0-2.
	   Bits 2-0: Block number.

	   SAA6588 byte order is Status-MSB-LSB, so we have to swap the
	   first and the last of the 3 bytes block.
	 */

	swap(tmpbuf[2], tmpbuf[0]);

	/* Map 'Invalid block E' to 'Invalid Block' */
	if (blocknum == 6)
		blocknum = V4L2_RDS_BLOCK_INVALID;
	/* And if are not in mmbs mode, then 'Block E' is also mapped
	   to 'Invalid Block'. As far as I can tell MMBS is discontinued,
	   and if there is ever a need to support E blocks, then please
	   contact the linux-media mailinglist. */
	else if (!mmbs && blocknum == 5)
		blocknum = V4L2_RDS_BLOCK_INVALID;
	tmp = blocknum;
	tmp |= blocknum << 3;	/* Received offset == Offset Name (OK ?) */
	if ((tmpbuf[2] & 0x03) == 0x03)
		tmp |= V4L2_RDS_BLOCK_ERROR;	 /* uncorrectable error */
	else if ((tmpbuf[2] & 0x03) != 0x00)
		tmp |= V4L2_RDS_BLOCK_CORRECTED; /* corrected error */
	tmpbuf[2] = tmp;	/* Is this enough ? Should we also check other bits ? */

	spin_lock_irqsave(&s->lock, flags);
	block_to_buf(s, tmpbuf);
	spin_unlock_irqrestore(&s->lock, flags);
	s->data_available_for_read = 1;
	wake_up_interruptible(&s->read_queue);
}
