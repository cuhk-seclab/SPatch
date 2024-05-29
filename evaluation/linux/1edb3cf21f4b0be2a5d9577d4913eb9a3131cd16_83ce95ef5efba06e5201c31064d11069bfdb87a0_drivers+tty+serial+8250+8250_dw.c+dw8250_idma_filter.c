static bool dw8250_idma_filter(struct dma_chan *chan, void *param)
{
	return param == chan->device->dev->parent;
}
