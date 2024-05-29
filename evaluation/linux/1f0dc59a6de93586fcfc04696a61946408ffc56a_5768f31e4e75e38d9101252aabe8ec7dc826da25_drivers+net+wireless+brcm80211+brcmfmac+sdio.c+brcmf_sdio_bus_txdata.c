static int brcmf_sdio_bus_txdata(struct device *dev, struct sk_buff *pkt)
{
	int ret = -EBADE;
	uint prec;
	struct brcmf_bus *bus_if = dev_get_drvdata(dev);
	struct brcmf_sdio_dev *sdiodev = bus_if->bus_priv.sdio;
	struct brcmf_sdio *bus = sdiodev->bus;

	brcmf_dbg(TRACE, "Enter: pkt: data %p len %d\n", pkt->data, pkt->len);
	if (sdiodev->state != BRCMF_SDIOD_DATA)
		return -EIO;

	/* Add space for the header */
	skb_push(pkt, bus->tx_hdrlen);
	/* precondition: IS_ALIGNED((unsigned long)(pkt->data), 2) */

	prec = prio2prec((pkt->priority & PRIOMASK));

	/* Check for existing queue, current flow-control,
			 pending event, or pending clock */
	brcmf_dbg(TRACE, "deferring pktq len %d\n", pktq_len(&bus->txq));
	bus->sdcnt.fcqueued++;

	/* Priority based enq */
	spin_lock_bh(&bus->txq_lock);
	/* reset bus_flags in packet cb */
	*(u16 *)(pkt->cb) = 0;
	if (!brcmf_sdio_prec_enq(&bus->txq, pkt, prec)) {
		skb_pull(pkt, bus->tx_hdrlen);
		brcmf_err("out of bus->txq !!!\n");
		ret = -ENOSR;
	} else {
		ret = 0;
	}

	if (pktq_len(&bus->txq) >= TXHI) {
		bus->txoff = true;
		brcmf_txflowblock(dev, true);
	}
	spin_unlock_bh(&bus->txq_lock);

#ifdef DEBUG
	if (pktq_plen(&bus->txq, prec) > qcount[prec])
		qcount[prec] = pktq_plen(&bus->txq, prec);
#endif

	brcmf_sdio_trigger_dpc(bus);
	return ret;
}
