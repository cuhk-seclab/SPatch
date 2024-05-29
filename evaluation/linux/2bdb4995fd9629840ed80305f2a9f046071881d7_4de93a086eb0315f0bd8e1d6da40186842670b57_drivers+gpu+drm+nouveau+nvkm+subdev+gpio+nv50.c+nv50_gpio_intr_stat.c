static void
nv50_gpio_intr_stat(struct nvkm_gpio *gpio, u32 *hi, u32 *lo)
{
	u32 intr = nvkm_rd32(device, 0x00e054);
	u32 intr = nv_rd32(gpio, 0x00e054);
	u32 stat = nv_rd32(gpio, 0x00e050) & intr;
	*lo = (stat & 0xffff0000) >> 16;
	*hi = (stat & 0x0000ffff);
	nv_wr32(gpio, 0x00e054, intr);
}
