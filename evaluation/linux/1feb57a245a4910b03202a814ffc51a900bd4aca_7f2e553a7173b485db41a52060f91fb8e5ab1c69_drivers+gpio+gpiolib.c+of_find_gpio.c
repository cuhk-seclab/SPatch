static struct gpio_desc *of_find_gpio(struct device *dev, const char *con_id,
				      unsigned int idx,
				      enum gpio_lookup_flags *flags)
{
	static const char * const suffixes[] = { "gpios", "gpio" };
	char prop_name[32]; /* 32 is max size of property name */
	enum of_gpio_flags of_flags;
	struct gpio_desc *desc;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(suffixes); i++) {
		if (con_id)
			snprintf(prop_name, sizeof(prop_name), "%s-%s", con_id,
							       suffixes[i]);
		else
			snprintf(prop_name, sizeof(prop_name), "%s",
							       suffixes[i]);

		desc = of_get_named_gpiod_flags(dev->of_node, prop_name, idx,
						&of_flags);
		if (!IS_ERR(desc) || (PTR_ERR(desc) == -EPROBE_DEFER))
			break;
	}

	if (IS_ERR(desc))
		return desc;

	if (of_flags & OF_GPIO_ACTIVE_LOW)
		*flags |= GPIO_ACTIVE_LOW;

	return desc;
}
