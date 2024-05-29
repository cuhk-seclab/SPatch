static int dw_probe(struct platform_device *pdev)
{
	struct dw_dma_chip *chip;
	struct device *dev = &pdev->dev;
	struct resource *mem;
	struct dw_dma_platform_data *pdata;
	int err;

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->irq = platform_get_irq(pdev, 0);
	if (chip->irq < 0)
		return chip->irq;

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	chip->regs = devm_ioremap_resource(dev, mem);
	if (IS_ERR(chip->regs))
		return PTR_ERR(chip->regs);

	err = dma_coerce_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (err)
		return err;

	pdata = dev_get_platdata(dev);
	if (!pdata)
		pdata = dw_dma_parse_dt(pdev);

	chip->dev = dev;

	chip->clk = devm_clk_get(chip->dev, "hclk");
	if (IS_ERR(chip->clk))
		return PTR_ERR(chip->clk);
	err = clk_prepare_enable(chip->clk);
	if (err)
		return err;

	pm_runtime_enable(&pdev->dev);

	err = dw_dma_probe(chip, pdata);
	if (err)
		goto err_dw_dma_probe;

	platform_set_drvdata(pdev, chip);

	if (pdev->dev.of_node) {
		err = of_dma_controller_register(pdev->dev.of_node,
						 dw_dma_of_xlate, chip->dw);
		if (err)
			dev_err(&pdev->dev,
				"could not register of_dma_controller\n");
	}

	if (ACPI_HANDLE(&pdev->dev))
		dw_dma_acpi_controller_register(chip->dw);

	return 0;

err_dw_dma_probe:
	pm_runtime_disable(&pdev->dev);
	clk_disable_unprepare(chip->clk);
	return err;
}
