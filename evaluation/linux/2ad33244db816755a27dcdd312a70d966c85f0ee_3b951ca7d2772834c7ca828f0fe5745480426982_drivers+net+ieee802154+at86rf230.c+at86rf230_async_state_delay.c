static void
at86rf230_async_state_delay(void *context)
{
	struct at86rf230_state_change *ctx = context;
	struct at86rf230_local *lp = ctx->lp;
	struct at86rf2xx_chip_data *c = lp->data;
	bool force = false;
	ktime_t tim;

	/* The force state changes are will show as normal states in the
	 * state status subregister. We change the to_state to the
	 * corresponding one and remember if it was a force change, this
	 * differs if we do a state change from STATE_BUSY_RX_AACK.
	 */
	switch (ctx->to_state) {
	case STATE_FORCE_TX_ON:
		ctx->to_state = STATE_TX_ON;
		force = true;
		break;
	case STATE_FORCE_TRX_OFF:
		ctx->to_state = STATE_TRX_OFF;
		force = true;
		break;
	default:
		break;
	}

	switch (ctx->from_state) {
	case STATE_TRX_OFF:
		switch (ctx->to_state) {
		case STATE_RX_AACK_ON:
			tim = ktime_set(0, c->t_off_to_aack * NSEC_PER_USEC);
			/* state change from TRX_OFF to RX_AACK_ON to do a
			 * calibration, we need to reset the timeout for the
			 * next one.
			 */
			lp->cal_timeout = jiffies + AT86RF2XX_CAL_LOOP_TIMEOUT;
			goto change;
		case STATE_TX_ARET_ON:
		case STATE_TX_ON:
			tim = ktime_set(0, c->t_off_to_tx_on * NSEC_PER_USEC);
			/* state change from TRX_OFF to TX_ON or ARET_ON to do
			 * a calibration, we need to reset the timeout for the
			 * next one.
			 */
			lp->cal_timeout = jiffies + AT86RF2XX_CAL_LOOP_TIMEOUT;
			goto change;
		default:
			break;
		}
		break;
	case STATE_BUSY_RX_AACK:
		switch (ctx->to_state) {
		case STATE_TRX_OFF:
		case STATE_TX_ON:
			/* Wait for worst case receiving time if we
			 * didn't make a force change from BUSY_RX_AACK
			 * to TX_ON or TRX_OFF.
			 */
			if (!force) {
				tim = ktime_set(0, (c->t_frame + c->t_p_ack) *
						   NSEC_PER_USEC);
				goto change;
			}
			break;
		default:
			break;
		}
		break;
	/* Default value, means RESET state */
	case STATE_P_ON:
		switch (ctx->to_state) {
		case STATE_TRX_OFF:
			tim = ktime_set(0, c->t_reset_to_off * NSEC_PER_USEC);
			goto change;
		default:
			break;
		}
		break;
	default:
		break;
	}

	/* Default delay is 1us in the most cases */
	tim = ktime_set(0, NSEC_PER_USEC);

change:
	hrtimer_start(&ctx->timer, tim, HRTIMER_MODE_REL);
}
