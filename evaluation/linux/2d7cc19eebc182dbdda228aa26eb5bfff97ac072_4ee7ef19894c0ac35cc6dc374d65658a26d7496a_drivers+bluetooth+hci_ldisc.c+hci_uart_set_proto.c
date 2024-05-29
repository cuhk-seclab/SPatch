static int hci_uart_set_proto(struct hci_uart *hu, int id)
{
	const struct hci_uart_proto *p;
	int err;

	p = hci_uart_get_proto(id);
	if (!p)
		return -EPROTONOSUPPORT;

	err = p->open(hu);
	if (err)
		return err;

	hu->proto = p;

	err = hci_uart_register_dev(hu);
	if (err) {
		p->close(hu);
		return err;
	}

	return 0;
}
