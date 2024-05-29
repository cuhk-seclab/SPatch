static void
at86rf230_async_read_reg(struct at86rf230_local *lp, u8 reg,
			 struct at86rf230_state_change *ctx,
			 void (*complete)(void *context))
{
	int rc;

	u8 *tx_buf = ctx->buf;

	tx_buf[0] = (reg & CMD_REG_MASK) | CMD_REG;
	ctx->msg.complete = complete;
	rc = spi_async(lp->spi, &ctx->msg);
	if (rc)
		at86rf230_async_error(lp, ctx, rc);
}
