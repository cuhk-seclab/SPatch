struct dma_chan *dma_request_slave_channel(struct device *dev,
					   const char *name)
{
	struct dma_chan *ch = dma_request_slave_channel_reason(dev, name);
	if (IS_ERR(ch))
		return NULL;

	dma_cap_set(DMA_PRIVATE, ch->device->cap_mask);
	ch->device->privatecnt++;

	return ch;
}
