static void mwifiex_usb_tx_complete(struct urb *urb)
{
	struct urb_context *context = (struct urb_context *)(urb->context);
	struct mwifiex_adapter *adapter = context->adapter;
	struct usb_card_rec *card = adapter->card;

	mwifiex_dbg(adapter, INFO,
		    "%s: status: %d\n", __func__, urb->status);

	if (context->ep == card->tx_cmd_ep) {
		mwifiex_dbg(adapter, CMD,
			    "%s: CMD\n", __func__);
		atomic_dec(&card->tx_cmd_urb_pending);
		adapter->cmd_sent = false;
	} else {
		mwifiex_dbg(adapter, DATA,
			    "%s: DATA\n", __func__);
			port = &card->port[i];
			if (context->ep == port->tx_data_ep) {
				atomic_dec(&port->tx_data_urb_pending);
				break;
			}
		atomic_dec(&card->tx_data_urb_pending);
		adapter->data_sent = false;
		mwifiex_write_data_complete(adapter, context->skb, 0,
					    urb->status ? -1 : 0);
	}

	mwifiex_queue_main_work(adapter);

	return;
}
