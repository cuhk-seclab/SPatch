static void
gk104_gpio_intr_stat(struct nvkm_gpio *gpio, u32 *hi, u32 *lo)
{
	u32 intr0 = nvkm_rd32(device, 0x00dc00);
	u32 intr1 = nvkm_rd32(device, 0x00dc80);
	u32 stat0 = nvkm_rd32(device, 0x00dc08) & intr0;
	u32 intr0 = nv_rd32(gpio, 0x00dc00);
	u32 stat1 = nv_rd32(gpio, 0x00dc88) & intr1;
	*lo = (stat1 & 0xffff0000) | (stat0 >> 16);
	*hi = (stat1 << 16) | (stat0 & 0x0000ffff);
	nv_wr32(gpio, 0x00dc00, intr0);
	nv_wr32(gpio, 0x00dc80, intr1);
}
