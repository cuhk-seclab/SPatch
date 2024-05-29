void xhci_stop(struct usb_hcd *hcd)
{
	u32 temp;
	struct xhci_hcd *xhci = hcd_to_xhci(hcd);

	mutex_lock(&xhci->mutex);

	if (!usb_hcd_is_primary_hcd(hcd)) {
		xhci_only_stop_hcd(xhci->shared_hcd);
		mutex_unlock(&xhci->mutex);
		return;
	}

	spin_lock_irq(&xhci->lock);
	/* Make sure the xHC is halted for a USB3 roothub
	 * (xhci_stop() could be called as part of failed init).
	 */
	xhci_halt(xhci);
	xhci_reset(xhci);
	spin_unlock_irq(&xhci->lock);

	xhci_cleanup_msix(xhci);

	/* Deleting Compliance Mode Recovery Timer */
	if ((xhci->quirks & XHCI_COMP_MODE_QUIRK) &&
			(!(xhci_all_ports_seen_u0(xhci)))) {
		del_timer_sync(&xhci->comp_mode_recovery_timer);
		xhci_dbg_trace(xhci, trace_xhci_dbg_quirks,
				"%s: compliance mode recovery timer deleted",
				__func__);
	}

	if (xhci->quirks & XHCI_AMD_PLL_FIX)
		usb_amd_dev_put();

	xhci_dbg_trace(xhci, trace_xhci_dbg_init,
			"// Disabling event ring interrupts");
	temp = readl(&xhci->op_regs->status);
	writel(temp & ~STS_EINT, &xhci->op_regs->status);
	temp = readl(&xhci->ir_set->irq_pending);
	writel(ER_IRQ_DISABLE(temp), &xhci->ir_set->irq_pending);
	xhci_print_ir_set(xhci, 0);

	xhci_dbg_trace(xhci, trace_xhci_dbg_init, "cleaning up memory");
	xhci_mem_cleanup(xhci);
	xhci_dbg_trace(xhci, trace_xhci_dbg_init,
			"xhci_stop completed - status = %x",
			readl(&xhci->op_regs->status));
	mutex_unlock(&xhci->mutex);
}
