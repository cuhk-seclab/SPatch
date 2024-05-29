static int pinmux_func_name_to_selector(struct pinctrl_dev *pctldev,
					const char *function)
{
	const struct pinmux_ops *ops = pctldev->desc->pmxops;
	unsigned nfuncs = ops->get_functions_count(pctldev);
	unsigned selector = 0;

	/* See if this pctldev has this function */
	while (selector < nfuncs) {
		const char *fname = ops->get_function_name(pctldev,
							   selector);

		if (!strcmp(function, fname))
			return selector;

		selector++;
	}

	dev_err(pctldev->dev, "function '%s' not supported\n", function);
	return -EINVAL;
}
