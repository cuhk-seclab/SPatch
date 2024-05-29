static inline void i40e_update_enable_itr(struct i40e_vsi *vsi,
					  struct i40e_q_vector *q_vector)
{
	struct i40e_hw *hw = &vsi->back->hw;
	bool rx = false, tx = false;
	u32 rxval, txval;
	int vector;

	vector = (q_vector->v_idx + vsi->base_vector);

	/* avoid dynamic calculation if in countdown mode OR if
	 * all dynamic is disabled
	 */
	rxval = txval = i40e_buildreg_itr(I40E_ITR_NONE, 0);

	if (q_vector->itr_countdown > 0 ||
	    (!ITR_IS_DYNAMIC(vsi->rx_itr_setting) &&
	     !ITR_IS_DYNAMIC(vsi->tx_itr_setting))) {
		goto enable_int;
	}

	if (ITR_IS_DYNAMIC(vsi->rx_itr_setting)) {
		rx = i40e_set_new_dynamic_itr(&q_vector->rx);
		rxval = i40e_buildreg_itr(I40E_RX_ITR, q_vector->rx.itr);
	}

	if (ITR_IS_DYNAMIC(vsi->tx_itr_setting)) {
		tx = i40e_set_new_dynamic_itr(&q_vector->tx);
		txval = i40e_buildreg_itr(I40E_TX_ITR, q_vector->tx.itr);
	}

	if (rx || tx) {
		/* get the higher of the two ITR adjustments and
		 * use the same value for both ITR registers
		 * when in adaptive mode (Rx and/or Tx)
		 */
		u16 itr = max(q_vector->tx.itr, q_vector->rx.itr);

		q_vector->tx.itr = q_vector->rx.itr = itr;
		txval = i40e_buildreg_itr(I40E_TX_ITR, itr);
		tx = true;
		rxval = i40e_buildreg_itr(I40E_RX_ITR, itr);
		rx = true;
	}

	/* only need to enable the interrupt once, but need
	 * to possibly update both ITR values
	 */
	if (rx) {
		/* set the INTENA_MSK_MASK so that this first write
		 * won't actually enable the interrupt, instead just
		 * updating the ITR (it's bit 31 PF and VF)
		 */
		rxval |= BIT(31);
		/* don't check _DOWN because interrupt isn't being enabled */
		wr32(hw, INTREG(vector - 1), rxval);
	}

enable_int:
	if (!test_bit(__I40E_DOWN, &vsi->state))
		wr32(hw, INTREG(vector - 1), txval);

	if (q_vector->itr_countdown)
		q_vector->itr_countdown--;
	else
		q_vector->itr_countdown = ITR_COUNTDOWN_START;
}
