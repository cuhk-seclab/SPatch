static void prism2sta_disconnect_usb(struct usb_interface *interface)
{
	wlandevice_t *wlandev;

	wlandev = (wlandevice_t *)usb_get_intfdata(interface);
	if (wlandev != NULL) {
		LIST_HEAD(cleanlist);
		struct list_head *entry;
		struct list_head *temp;
		unsigned long flags;

		hfa384x_t *hw = wlandev->priv;

		if (!hw)
			goto exit;

		spin_lock_irqsave(&hw->ctlxq.lock, flags);

		p80211netdev_hwremoved(wlandev);
		list_splice_init(&hw->ctlxq.reapable, &cleanlist);
		list_splice_init(&hw->ctlxq.completing, &cleanlist);
		list_splice_init(&hw->ctlxq.pending, &cleanlist);
		list_splice_init(&hw->ctlxq.active, &cleanlist);

		spin_unlock_irqrestore(&hw->ctlxq.lock, flags);

		/* There's no hardware to shutdown, but the driver
		 * might have some tasks or tasklets that must be
		 * stopped before we can tear everything down.
		 */
		prism2sta_ifstate(wlandev, P80211ENUM_ifstate_disable);

		del_singleshot_timer_sync(&hw->throttle);
		del_singleshot_timer_sync(&hw->reqtimer);
		del_singleshot_timer_sync(&hw->resptimer);

		/* Unlink all the URBs. This "removes the wheels"
		 * from the entire CTLX handling mechanism.
		 */
		usb_kill_urb(&hw->rx_urb);
		usb_kill_urb(&hw->tx_urb);
		usb_kill_urb(&hw->ctlx_urb);

		tasklet_kill(&hw->completion_bh);
		tasklet_kill(&hw->reaper_bh);

		flush_scheduled_work();

		/* Now we complete any outstanding commands
		 * and tell everyone who is waiting for their
		 * responses that we have shut down.
		 */
		list_for_each(entry, &cleanlist) {
			hfa384x_usbctlx_t *ctlx;

			ctlx = list_entry(entry, hfa384x_usbctlx_t, list);
			complete(&ctlx->done);
		}

		/* Give any outstanding synchronous commands
		 * a chance to complete. All they need to do
		 * is "wake up", so that's easy.
		 * (I'd like a better way to do this, really.)
		 */
		msleep(100);

		/* Now delete the CTLXs, because no-one else can now. */
		list_for_each_safe(entry, temp, &cleanlist) {
			hfa384x_usbctlx_t *ctlx;

			ctlx = list_entry(entry, hfa384x_usbctlx_t, list);
			kfree(ctlx);
		}

		/* Unhook the wlandev */
		unregister_wlandev(wlandev);
		wlan_unsetup(wlandev);

		usb_put_dev(hw->usb);

		hfa384x_destroy(hw);
		kfree(hw);

		kfree(wlandev);
	}

exit:
	usb_set_intfdata(interface, NULL);
}
