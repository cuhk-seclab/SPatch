int
gf110_gpio_drive(struct nvkm_gpio *gpio, int line, int dir, int out)
{
	struct nvkm_device *device = gpio->subdev.device;
	u32 data = ((dir ^ 1) << 13) | (out << 12);
	nvkm_mask(device, 0x00d610 + (line * 4), 0x00003000, data);
	nvkm_mask(device, 0x00d604, 0x00000001, 0x00000001); /* update? */
	return 0;
}
