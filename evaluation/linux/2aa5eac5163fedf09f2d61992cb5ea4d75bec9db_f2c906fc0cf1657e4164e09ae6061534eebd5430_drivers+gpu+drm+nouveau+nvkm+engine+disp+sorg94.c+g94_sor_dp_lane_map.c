u32
g94_sor_dp_lane_map(struct nvkm_device *device, u8 lane)
{
	static const u8 gm100[] = { 0, 8, 16, 24 };
	static const u8 mcp89[] = { 24, 16, 8, 0 }; /* thanks, apple.. */
	static const u8   g94[] = { 16, 8, 0, 24 };
	if (device->chipset >= 0x110)
		return gm100[lane];
	if (device->chipset == 0xaf)
		return mcp89[lane];
	return g94[lane];
}
