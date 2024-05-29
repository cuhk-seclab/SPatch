irqreturn_t
mISDNipac_irq(struct ipac_hw *ipac, int maxloop)
{
	int cnt = maxloop + 1;
	u8 ista, istad;
	struct isac_hw  *isac = &ipac->isac;

	if (ipac->type & IPAC_TYPE_IPACX) {
		ista = ReadIPAC(ipac, ISACX_ISTA);
		while (ista && cnt--) {
			pr_debug("%s: ISTA %02x\n", ipac->name, ista);
			if (ista & IPACX__ICA)
				ipac_irq(&ipac->hscx[0], ista);
			if (ista & IPACX__ICB)
				ipac_irq(&ipac->hscx[1], ista);
			if (ista & (ISACX__ICD | ISACX__CIC))
				mISDNisac_irq(&ipac->isac, ista);
			ista = ReadIPAC(ipac, ISACX_ISTA);
		}
	} else if (ipac->type & IPAC_TYPE_IPAC) {
		ista = ReadIPAC(ipac, IPAC_ISTA);
		while (ista && cnt--) {
			pr_debug("%s: ISTA %02x\n", ipac->name, ista);
			if (ista & (IPAC__ICD | IPAC__EXD)) {
				istad = ReadISAC(isac, ISAC_ISTA);
				pr_debug("%s: ISTAD %02x\n", ipac->name, istad);
				if (istad & IPAC_D_TIN2)
					pr_debug("%s TIN2 irq\n", ipac->name);
				if (ista & IPAC__EXD)
					istad |= 1; /* ISAC EXI */
				mISDNisac_irq(isac, istad);
			}
			if (ista & (IPAC__ICA | IPAC__EXA))
				ipac_irq(&ipac->hscx[0], ista);
			if (ista & (IPAC__ICB | IPAC__EXB))
				ipac_irq(&ipac->hscx[1], ista);
			ista = ReadIPAC(ipac, IPAC_ISTA);
		}
	} else if (ipac->type & IPAC_TYPE_HSCX) {
		while (cnt) {
			ista = ReadIPAC(ipac, IPAC_ISTAB + ipac->hscx[1].off);
			pr_debug("%s: B2 ISTA %02x\n", ipac->name, ista);
			if (ista)
				ipac_irq(&ipac->hscx[1], ista);
			istad = ReadISAC(isac, ISAC_ISTA);
			pr_debug("%s: ISTAD %02x\n", ipac->name, istad);
			if (istad)
				mISDNisac_irq(isac, istad);
			if (0 == (ista | istad))
				break;
			cnt--;
		}
	}
	if (cnt > maxloop) /* only for ISAC/HSCX without PCI IRQ test */
		return IRQ_NONE;
	if (cnt < maxloop)
		pr_debug("%s: %d irqloops cpu%d\n", ipac->name,
			 maxloop - cnt, smp_processor_id());
	if (maxloop && !cnt)
		pr_notice("%s: %d IRQ LOOP cpu%d\n", ipac->name,
			  maxloop, smp_processor_id());
	return IRQ_HANDLED;
}
