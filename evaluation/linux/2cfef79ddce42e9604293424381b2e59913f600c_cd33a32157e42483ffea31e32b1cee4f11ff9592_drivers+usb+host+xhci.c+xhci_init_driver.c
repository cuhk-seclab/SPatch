void xhci_init_driver(struct hc_driver *drv, int (*setup_fn)(struct usb_hcd *))
{
	BUG_ON(!setup_fn);
	*drv = xhci_hc_driver;
	drv->reset = setup_fn;
}
