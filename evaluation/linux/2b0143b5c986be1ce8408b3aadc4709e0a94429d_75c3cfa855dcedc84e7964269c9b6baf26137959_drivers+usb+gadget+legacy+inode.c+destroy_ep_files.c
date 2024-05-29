static void destroy_ep_files (struct dev_data *dev)
{
	DBG (dev, "%s %d\n", __func__, dev->state);

	/* dev->state must prevent interference */
	spin_lock_irq (&dev->lock);
	while (!list_empty(&dev->epfiles)) {
		struct ep_data	*ep;
		struct inode	*parent;
		struct dentry	*dentry;

		/* break link to FS */
		ep = list_first_entry (&dev->epfiles, struct ep_data, epfiles);
		list_del_init (&ep->epfiles);
		dentry = ep->dentry;
		ep->dentry = NULL;
		parent = dentry->d_parent->d_inode;

		/* break link to controller */
		if (ep->state == STATE_EP_ENABLED)
			(void) usb_ep_disable (ep->ep);
		ep->state = STATE_EP_UNBOUND;
		usb_ep_free_request (ep->ep, ep->req);
		ep->ep = NULL;
		wake_up (&ep->wait);
		put_ep (ep);

		spin_unlock_irq (&dev->lock);

		/* break link to dcache */
		mutex_lock (&parent->i_mutex);
		d_delete (dentry);
		dput (dentry);
		mutex_unlock (&parent->i_mutex);

		spin_lock_irq (&dev->lock);
	}
	spin_unlock_irq (&dev->lock);
}
