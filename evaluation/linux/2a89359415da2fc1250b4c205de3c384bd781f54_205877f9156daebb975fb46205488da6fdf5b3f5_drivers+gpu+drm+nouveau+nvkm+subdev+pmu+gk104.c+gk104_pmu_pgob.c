static void
gk104_pmu_pgob(struct nvkm_pmu *pmu, bool enable)
{
	struct nvkm_device *device = nv_device(pmu);
	struct nvkm_object *dev = nv_object(device);

	nv_mask(pmu, 0x000200, 0x00001000, 0x00000000);
	nv_rd32(pmu, 0x000200);
	nv_mask(pmu, 0x000200, 0x08000000, 0x08000000);
	msleep(50);

	nv_mask(pmu, 0x10a78c, 0x00000002, 0x00000002);
	nv_mask(pmu, 0x10a78c, 0x00000001, 0x00000001);
	nv_mask(pmu, 0x10a78c, 0x00000001, 0x00000000);

	nv_mask(pmu, 0x020004, 0xc0000000, enable ? 0xc0000000 : 0x40000000);
	msleep(50);

	nv_mask(pmu, 0x10a78c, 0x00000002, 0x00000000);
	nv_mask(pmu, 0x10a78c, 0x00000001, 0x00000001);
	nv_mask(pmu, 0x10a78c, 0x00000001, 0x00000000);

	nv_mask(pmu, 0x000200, 0x08000000, 0x00000000);
	nv_mask(pmu, 0x000200, 0x00001000, 0x00001000);
	nv_rd32(pmu, 0x000200);

	if (nv_device_match(dev, 0x11fc, 0x17aa, 0x2211) /* Lenovo W541 */
	 || nv_device_match(dev, 0x11fc, 0x17aa, 0x221e) /* Lenovo W541 */
	 || nvkm_boolopt(device->cfgopt, "War00C800_0", false)) {
		nv_info(pmu, "hw bug workaround enabled\n");
		switch (device->chipset) {
		case 0xe4:
			magic(pmu, 0x04000000);
			magic(pmu, 0x06000000);
			magic(pmu, 0x0c000000);
			magic(pmu, 0x0e000000);
			break;
		case 0xe6:
			magic(pmu, 0x02000000);
			magic(pmu, 0x04000000);
			magic(pmu, 0x0a000000);
			break;
		case 0xe7:
			magic(pmu, 0x02000000);
			break;
		default:
			break;
		}
	}
}
