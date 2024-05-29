static void kvaser_usb_remove_interfaces(struct kvaser_usb *dev)
{
	int i;

	for (i = 0; i < dev->nchannels; i++) {
		if (!dev->nets[i])
			continue;

		unregister_netdev(dev->nets[i]->netdev);
	}

	kvaser_usb_unlink_all_urbs(dev);

	for (i = 0; i < dev->nchannels; i++) {
		if (!dev->nets[i])
			continue;

		free_candev(dev->nets[i]->netdev);
	}
}
