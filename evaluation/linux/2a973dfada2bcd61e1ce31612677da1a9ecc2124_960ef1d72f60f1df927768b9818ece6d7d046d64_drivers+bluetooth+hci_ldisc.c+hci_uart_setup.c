static int hci_uart_setup(struct hci_dev *hdev)
{
	struct hci_uart *hu = hci_get_drvdata(hdev);
	struct hci_rp_read_local_version *ver;
	struct sk_buff *skb;
	unsigned int speed;
	int err;

	/* Init speed if any */
	if (hu->init_speed)
		speed = hu->init_speed;
	else if (hu->proto->init_speed)
		speed = hu->proto->init_speed;
	else
		speed = 0;

	if (speed)
		hci_uart_set_baudrate(hu, speed);

	/* Operational speed if any */
	if (hu->oper_speed)
		speed = hu->oper_speed;
	else if (hu->proto->oper_speed)
		speed = hu->proto->oper_speed;
	else
		speed = 0;

	if (hu->proto->set_baudrate && speed) {
		err = hu->proto->set_baudrate(hu, speed);
		if (!err)
			hci_uart_set_baudrate(hu, speed);
	}

	if (hu->proto->setup)
		return hu->proto->setup(hu);

	if (!test_bit(HCI_UART_VND_DETECT, &hu->hdev_flags))
		return 0;

	skb = __hci_cmd_sync(hdev, HCI_OP_READ_LOCAL_VERSION, 0, NULL,
			     HCI_INIT_TIMEOUT);
	if (IS_ERR(skb)) {
		BT_ERR("%s: Reading local version information failed (%ld)",
		       hdev->name, PTR_ERR(skb));
		return 0;
	}

	if (skb->len != sizeof(*ver)) {
		BT_ERR("%s: Event length mismatch for version information",
		       hdev->name);
		goto done;
	}

	ver = (struct hci_rp_read_local_version *)skb->data;

	switch (le16_to_cpu(ver->manufacturer)) {
#ifdef CONFIG_BT_HCIUART_INTEL
	case 2:
		hdev->set_bdaddr = btintel_set_bdaddr;
		btintel_check_bdaddr(hdev);
		break;
#endif
#ifdef CONFIG_BT_HCIUART_BCM
	case 15:
		hdev->set_bdaddr = btbcm_set_bdaddr;
		btbcm_check_bdaddr(hdev);
		break;
#endif
	}

done:
	kfree_skb(skb);
	return 0;
}
