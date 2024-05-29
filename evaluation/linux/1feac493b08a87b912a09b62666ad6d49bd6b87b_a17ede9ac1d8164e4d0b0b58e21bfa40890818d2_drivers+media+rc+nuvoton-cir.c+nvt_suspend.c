static int nvt_suspend(struct pnp_dev *pdev, pm_message_t state)
{
	struct nvt_dev *nvt = pnp_get_drvdata(pdev);
	unsigned long flags;

	nvt_dbg("%s called", __func__);

	/* zero out misc state tracking */
	spin_lock_irqsave(&nvt->nvt_lock, flags);
	nvt->study_state = ST_STUDY_NONE;
	nvt->wake_state = ST_WAKE_NONE;
	spin_unlock_irqrestore(&nvt->nvt_lock, flags);

	spin_lock_irqsave(&nvt->tx.lock, flags);
	nvt->tx.tx_state = ST_TX_NONE;
	spin_unlock_irqrestore(&nvt->tx.lock, flags);

	/* disable all CIR interrupts */
	nvt_cir_reg_write(nvt, 0, CIR_IREN);

	/* disable cir logical dev */
	nvt_disable_logical_dev(nvt, LOGICAL_DEV_CIR);

	/* make sure wake is enabled */
	nvt_enable_wake(nvt);

	return 0;
}
