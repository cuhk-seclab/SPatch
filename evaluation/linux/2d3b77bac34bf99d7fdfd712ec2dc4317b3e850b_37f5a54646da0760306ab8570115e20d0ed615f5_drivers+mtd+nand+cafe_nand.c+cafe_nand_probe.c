static int cafe_nand_probe(struct pci_dev *pdev,
				     const struct pci_device_id *ent)
{
	struct mtd_info *mtd;
	struct cafe_priv *cafe;
	uint32_t ctrl;
	int err = 0;
	int old_dma;
	struct nand_buffers *nbuf;

	/* Very old versions shared the same PCI ident for all three
	   functions on the chip. Verify the class too... */
	if ((pdev->class >> 8) != PCI_CLASS_MEMORY_FLASH)
		return -ENODEV;

	err = pci_enable_device(pdev);
	if (err)
		return err;

	pci_set_master(pdev);

	cafe = kzalloc(sizeof(*cafe), GFP_KERNEL);
	if (!cafe)
		return  -ENOMEM;

	mtd = nand_to_mtd(&cafe->nand);
	mtd->dev.parent = &pdev->dev;
	cafe->nand.priv = cafe;

	cafe->pdev = pdev;
	cafe->mmio = pci_iomap(pdev, 0, 0);
	if (!cafe->mmio) {
		dev_warn(&pdev->dev, "failed to iomap\n");
		err = -ENOMEM;
		goto out_free_mtd;
	}

	cafe->rs = init_rs_non_canonical(12, &cafe_mul, 0, 1, 8);
	if (!cafe->rs) {
		err = -ENOMEM;
		goto out_ior;
	}

	cafe->nand.cmdfunc = cafe_nand_cmdfunc;
	cafe->nand.dev_ready = cafe_device_ready;
	cafe->nand.read_byte = cafe_read_byte;
	cafe->nand.read_buf = cafe_read_buf;
	cafe->nand.write_buf = cafe_write_buf;
	cafe->nand.select_chip = cafe_select_chip;

	cafe->nand.chip_delay = 0;

	/* Enable the following for a flash based bad block table */
	cafe->nand.bbt_options = NAND_BBT_USE_FLASH;
	cafe->nand.options = NAND_OWN_BUFFERS;

	if (skipbbt) {
		cafe->nand.options |= NAND_SKIP_BBTSCAN;
		cafe->nand.block_bad = cafe_nand_block_bad;
	}

	if (numtimings && numtimings != 3) {
		dev_warn(&cafe->pdev->dev, "%d timing register values ignored; precisely three are required\n", numtimings);
	}

	if (numtimings == 3) {
		cafe_dev_dbg(&cafe->pdev->dev, "Using provided timings (%08x %08x %08x)\n",
			     timing[0], timing[1], timing[2]);
	} else {
		timing[0] = cafe_readl(cafe, NAND_TIMING1);
		timing[1] = cafe_readl(cafe, NAND_TIMING2);
		timing[2] = cafe_readl(cafe, NAND_TIMING3);

		if (timing[0] | timing[1] | timing[2]) {
			cafe_dev_dbg(&cafe->pdev->dev, "Timing registers already set (%08x %08x %08x)\n",
				     timing[0], timing[1], timing[2]);
		} else {
			dev_warn(&cafe->pdev->dev, "Timing registers unset; using most conservative defaults\n");
			timing[0] = timing[1] = timing[2] = 0xffffffff;
		}
	}

	/* Start off by resetting the NAND controller completely */
	cafe_writel(cafe, 1, NAND_RESET);
	cafe_writel(cafe, 0, NAND_RESET);

	cafe_writel(cafe, timing[0], NAND_TIMING1);
	cafe_writel(cafe, timing[1], NAND_TIMING2);
	cafe_writel(cafe, timing[2], NAND_TIMING3);

	cafe_writel(cafe, 0xffffffff, NAND_IRQ_MASK);
	err = request_irq(pdev->irq, &cafe_nand_interrupt, IRQF_SHARED,
			  "CAFE NAND", mtd);
	if (err) {
		dev_warn(&pdev->dev, "Could not register IRQ %d\n", pdev->irq);
		goto out_ior;
	}

	/* Disable master reset, enable NAND clock */
	ctrl = cafe_readl(cafe, GLOBAL_CTRL);
	ctrl &= 0xffffeff0;
	ctrl |= 0x00007000;
	cafe_writel(cafe, ctrl | 0x05, GLOBAL_CTRL);
	cafe_writel(cafe, ctrl | 0x0a, GLOBAL_CTRL);
	cafe_writel(cafe, 0, NAND_DMA_CTRL);

	cafe_writel(cafe, 0x7006, GLOBAL_CTRL);
	cafe_writel(cafe, 0x700a, GLOBAL_CTRL);

	/* Enable NAND IRQ in global IRQ mask register */
	cafe_writel(cafe, 0x80000007, GLOBAL_IRQ_MASK);
	cafe_dev_dbg(&cafe->pdev->dev, "Control %x, IRQ mask %x\n",
		cafe_readl(cafe, GLOBAL_CTRL),
		cafe_readl(cafe, GLOBAL_IRQ_MASK));

	/* Do not use the DMA for the nand_scan_ident() */
	old_dma = usedma;
	usedma = 0;

	/* Scan to find existence of the device */
	if (nand_scan_ident(mtd, 2, NULL)) {
		err = -ENXIO;
		goto out_irq;
	}

	cafe->dmabuf = dma_alloc_coherent(&cafe->pdev->dev,
				2112 + sizeof(struct nand_buffers) +
				mtd->writesize + mtd->oobsize,
				&cafe->dmaaddr, GFP_KERNEL);
	if (!cafe->dmabuf) {
		err = -ENOMEM;
		goto out_irq;
	}
	cafe->nand.buffers = nbuf = (void *)cafe->dmabuf + 2112;

	/* Set up DMA address */
	cafe_writel(cafe, cafe->dmaaddr & 0xffffffff, NAND_DMA_ADDR0);
	if (sizeof(cafe->dmaaddr) > 4)
		/* Shift in two parts to shut the compiler up */
		cafe_writel(cafe, (cafe->dmaaddr >> 16) >> 16, NAND_DMA_ADDR1);
	else
		cafe_writel(cafe, 0, NAND_DMA_ADDR1);

	cafe_dev_dbg(&cafe->pdev->dev, "Set DMA address to %x (virt %p)\n",
		cafe_readl(cafe, NAND_DMA_ADDR0), cafe->dmabuf);

	/* this driver does not need the @ecccalc and @ecccode */
	nbuf->ecccalc = NULL;
	nbuf->ecccode = NULL;
	nbuf->databuf = (uint8_t *)(nbuf + 1);

	/* Restore the DMA flag */
	usedma = old_dma;

	cafe->ctl2 = 1<<27; /* Reed-Solomon ECC */
	if (mtd->writesize == 2048)
		cafe->ctl2 |= 1<<29; /* 2KiB page size */

	/* Set up ECC according to the type of chip we found */
	if (mtd->writesize == 2048) {
		cafe->nand.ecc.layout = &cafe_oobinfo_2048;
		cafe->nand.bbt_td = &cafe_bbt_main_descr_2048;
		cafe->nand.bbt_md = &cafe_bbt_mirror_descr_2048;
	} else if (mtd->writesize == 512) {
		cafe->nand.ecc.layout = &cafe_oobinfo_512;
		cafe->nand.bbt_td = &cafe_bbt_main_descr_512;
		cafe->nand.bbt_md = &cafe_bbt_mirror_descr_512;
	} else {
		printk(KERN_WARNING "Unexpected NAND flash writesize %d. Aborting\n",
		       mtd->writesize);
		goto out_free_dma;
	}
	cafe->nand.ecc.mode = NAND_ECC_HW_SYNDROME;
	cafe->nand.ecc.size = mtd->writesize;
	cafe->nand.ecc.bytes = 14;
	cafe->nand.ecc.strength = 4;
	cafe->nand.ecc.hwctl  = (void *)cafe_nand_bug;
	cafe->nand.ecc.calculate = (void *)cafe_nand_bug;
	cafe->nand.ecc.correct  = (void *)cafe_nand_bug;
	cafe->nand.ecc.write_page = cafe_nand_write_page_lowlevel;
	cafe->nand.ecc.write_oob = cafe_nand_write_oob;
	cafe->nand.ecc.read_page = cafe_nand_read_page;
	cafe->nand.ecc.read_oob = cafe_nand_read_oob;

	err = nand_scan_tail(mtd);
	if (err)
		goto out_free_dma;

	pci_set_drvdata(pdev, mtd);

	mtd->name = "cafe_nand";
	mtd_device_parse_register(mtd, part_probes, NULL, NULL, 0);

	goto out;

 out_free_dma:
	dma_free_coherent(&cafe->pdev->dev,
			2112 + sizeof(struct nand_buffers) +
			mtd->writesize + mtd->oobsize,
			cafe->dmabuf, cafe->dmaaddr);
 out_irq:
	/* Disable NAND IRQ in global IRQ mask register */
	cafe_writel(cafe, ~1 & cafe_readl(cafe, GLOBAL_IRQ_MASK), GLOBAL_IRQ_MASK);
	free_irq(pdev->irq, mtd);
 out_ior:
	pci_iounmap(pdev, cafe->mmio);
 out_free_mtd:
	kfree(cafe);
 out:
	return err;
}
