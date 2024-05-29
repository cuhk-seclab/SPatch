static int pci_bus_alloc_from_region(struct pci_bus *bus, struct resource *res,
		resource_size_t size, resource_size_t align,
		resource_size_t min, unsigned long type_mask,
		resource_size_t (*alignf)(void *,
					  const struct resource *,
					  resource_size_t,
					  resource_size_t),
		void *alignf_data,
		struct pci_bus_region *region)
{
	int i, ret;
	struct resource *r, avail;
	resource_size_t max;

	type_mask |= IORESOURCE_TYPE_BITS;

	pci_bus_for_each_resource(bus, r, i) {
		resource_size_t min_used = min;

		if (!r)
			continue;

		/* type_mask must match */
		if ((res->flags ^ r->flags) & type_mask)
			continue;

		/* We cannot allocate a non-prefetching resource
		   from a pre-fetching area */
		if ((r->flags & IORESOURCE_PREFETCH) &&
		    !(res->flags & IORESOURCE_PREFETCH))
			continue;

		avail = *r;
		pci_clip_resource_to_region(bus, &avail, region);

		/*
		 * "min" is typically PCIBIOS_MIN_IO or PCIBIOS_MIN_MEM to
		 * protect badly documented motherboard resources, but if
		 * this is an already-configured bridge window, its start
		 * overrides "min".
		 */
		if (avail.start)
			min_used = avail.start;

		max = avail.end;

		/* Ok, try it out.. */
		ret = allocate_resource(r, res, size, min_used, max,
					align, alignf, alignf_data);
		if (ret == 0)
			return 0;
	}
	return -ENOMEM;
}
