int
nvkm_device_ctor(const struct nvkm_device_func *func,
		 const struct nvkm_device_quirk *quirk,
		 void *dev, enum nv_bus_type type, u64 handle,
		 const char *name, const char *cfg, const char *dbg,
		 bool detect, bool mmio, u64 subdev_mask,
		 struct nvkm_device *device)
{
	struct nvkm_subdev *subdev;
	u64 mmio_base, mmio_size;
	u32 boot0, strap;
	void __iomem *map;
	int ret = -EEXIST;
	int i;

	mutex_lock(&nv_devices_mutex);
	if (nvkm_device_find_locked(handle))
		goto done;

	device->func = func;
	device->quirk = quirk;
	switch (type) {
	case NVKM_BUS_PCI:
		device->pdev = dev;
		device->dev = &device->pdev->dev;
		break;
	case NVKM_BUS_PLATFORM:
		device->platformdev = dev;
		device->dev = &device->platformdev->dev;
		break;
	}
	device->handle = handle;
	device->cfgopt = cfg;
	device->dbgopt = dbg;
	device->name = name;
	list_add_tail(&device->head, &nv_devices);
	device->debug = nvkm_dbgopt(device->dbgopt, "device");

	ret = nvkm_event_init(&nvkm_device_event_func, 1, 1, &device->event);
	if (ret)
		goto done;

	mmio_base = device->func->resource_addr(device, 0);
	mmio_size = device->func->resource_size(device, 0);

	/* identify the chipset, and determine classes of subdev/engines */
	if (detect) {
		map = ioremap(mmio_base, 0x102000);
		if (ret = -ENOMEM, map == NULL)
			goto done;

		/* switch mmio to cpu's native endianness */
#ifndef __BIG_ENDIAN
		if (ioread32_native(map + 0x000004) != 0x00000000) {
#else
		if (ioread32_native(map + 0x000004) == 0x00000000) {
#endif
			iowrite32_native(0x01000001, map + 0x000004);
			ioread32_native(map);
		}

		/* read boot0 and strapping information */
		boot0 = ioread32_native(map + 0x000000);
		strap = ioread32_native(map + 0x101000);
		iounmap(map);

		/* determine chipset and derive architecture from it */
		if ((boot0 & 0x1f000000) > 0) {
			device->chipset = (boot0 & 0x1ff00000) >> 20;
			device->chiprev = (boot0 & 0x000000ff);
			switch (device->chipset & 0x1f0) {
			case 0x010: {
				if (0x461 & (1 << (device->chipset & 0xf)))
					device->card_type = NV_10;
				else
					device->card_type = NV_11;
				device->chiprev = 0x00;
				break;
			}
			case 0x020: device->card_type = NV_20; break;
			case 0x030: device->card_type = NV_30; break;
			case 0x040:
			case 0x060: device->card_type = NV_40; break;
			case 0x050:
			case 0x080:
			case 0x090:
			case 0x0a0: device->card_type = NV_50; break;
			case 0x0c0:
			case 0x0d0: device->card_type = NV_C0; break;
			case 0x0e0:
			case 0x0f0:
			case 0x100: device->card_type = NV_E0; break;
			case 0x110:
			case 0x120: device->card_type = GM100; break;
			default:
				break;
			}
		} else
		if ((boot0 & 0xff00fff0) == 0x20004000) {
			if (boot0 & 0x00f00000)
				device->chipset = 0x05;
			else
				device->chipset = 0x04;
			device->card_type = NV_04;
		}

		switch (device->chipset) {
		case 0x004: device->chip = &nv4_chipset; break;
		case 0x005: device->chip = &nv5_chipset; break;
		case 0x010: device->chip = &nv10_chipset; break;
		case 0x011: device->chip = &nv11_chipset; break;
		case 0x015: device->chip = &nv15_chipset; break;
		case 0x017: device->chip = &nv17_chipset; break;
		case 0x018: device->chip = &nv18_chipset; break;
		case 0x01a: device->chip = &nv1a_chipset; break;
		case 0x01f: device->chip = &nv1f_chipset; break;
		case 0x020: device->chip = &nv20_chipset; break;
		case 0x025: device->chip = &nv25_chipset; break;
		case 0x028: device->chip = &nv28_chipset; break;
		case 0x02a: device->chip = &nv2a_chipset; break;
		case 0x030: device->chip = &nv30_chipset; break;
		case 0x031: device->chip = &nv31_chipset; break;
		case 0x034: device->chip = &nv34_chipset; break;
		case 0x035: device->chip = &nv35_chipset; break;
		case 0x036: device->chip = &nv36_chipset; break;
		case 0x040: device->chip = &nv40_chipset; break;
		case 0x041: device->chip = &nv41_chipset; break;
		case 0x042: device->chip = &nv42_chipset; break;
		case 0x043: device->chip = &nv43_chipset; break;
		case 0x044: device->chip = &nv44_chipset; break;
		case 0x045: device->chip = &nv45_chipset; break;
		case 0x046: device->chip = &nv46_chipset; break;
		case 0x047: device->chip = &nv47_chipset; break;
		case 0x049: device->chip = &nv49_chipset; break;
		case 0x04a: device->chip = &nv4a_chipset; break;
		case 0x04b: device->chip = &nv4b_chipset; break;
		case 0x04c: device->chip = &nv4c_chipset; break;
		case 0x04e: device->chip = &nv4e_chipset; break;
		case 0x050: device->chip = &nv50_chipset; break;
		case 0x063: device->chip = &nv63_chipset; break;
		case 0x067: device->chip = &nv67_chipset; break;
		case 0x068: device->chip = &nv68_chipset; break;
		case 0x084: device->chip = &nv84_chipset; break;
		case 0x086: device->chip = &nv86_chipset; break;
		case 0x092: device->chip = &nv92_chipset; break;
		case 0x094: device->chip = &nv94_chipset; break;
		case 0x096: device->chip = &nv96_chipset; break;
		case 0x098: device->chip = &nv98_chipset; break;
		case 0x0a0: device->chip = &nva0_chipset; break;
		case 0x0a3: device->chip = &nva3_chipset; break;
		case 0x0a5: device->chip = &nva5_chipset; break;
		case 0x0a8: device->chip = &nva8_chipset; break;
		case 0x0aa: device->chip = &nvaa_chipset; break;
		case 0x0ac: device->chip = &nvac_chipset; break;
		case 0x0af: device->chip = &nvaf_chipset; break;
		case 0x0c0: device->chip = &nvc0_chipset; break;
		case 0x0c1: device->chip = &nvc1_chipset; break;
		case 0x0c3: device->chip = &nvc3_chipset; break;
		case 0x0c4: device->chip = &nvc4_chipset; break;
		case 0x0c8: device->chip = &nvc8_chipset; break;
		case 0x0ce: device->chip = &nvce_chipset; break;
		case 0x0cf: device->chip = &nvcf_chipset; break;
		case 0x0d7: device->chip = &nvd7_chipset; break;
		case 0x0d9: device->chip = &nvd9_chipset; break;
		case 0x0e4: device->chip = &nve4_chipset; break;
		case 0x0e6: device->chip = &nve6_chipset; break;
		case 0x0e7: device->chip = &nve7_chipset; break;
		case 0x0ea: device->chip = &nvea_chipset; break;
		case 0x0f0: device->chip = &nvf0_chipset; break;
		case 0x0f1: device->chip = &nvf1_chipset; break;
		case 0x106: device->chip = &nv106_chipset; break;
		case 0x108: device->chip = &nv108_chipset; break;
		case 0x117: device->chip = &nv117_chipset; break;
		case 0x124: device->chip = &nv124_chipset; break;
		case 0x126: device->chip = &nv126_chipset; break;
		case 0x12b: device->chip = &nv12b_chipset; break;
		default:
			nvdev_error(device, "unknown chipset (%08x)\n", boot0);
			goto done;
		}

		nvdev_info(device, "NVIDIA %s (%08x)\n",
			   device->chip->name, boot0);

		/* determine frequency of timing crystal */
		if ( device->card_type <= NV_10 || device->chipset < 0x17 ||
		    (device->chipset >= 0x20 && device->chipset < 0x25))
			strap &= 0x00000040;
		else
			strap &= 0x00400040;

		switch (strap) {
		case 0x00000000: device->crystal = 13500; break;
		case 0x00000040: device->crystal = 14318; break;
		case 0x00400000: device->crystal = 27000; break;
		case 0x00400040: device->crystal = 25000; break;
		}
	} else {
		device->chip = &null_chipset;
	}

	if (!device->name)
		device->name = device->chip->name;

	if (mmio) {
		device->pri = ioremap(mmio_base, mmio_size);
		if (!device->pri) {
			nvdev_error(device, "unable to map PRI\n");
			return -ENOMEM;
		}
	}

	mutex_init(&device->mutex);

	for (i = 0; i < NVKM_SUBDEV_NR; i++) {
#define _(s,m) case s:                                                         \
	if (device->chip->m && (subdev_mask & (1ULL << (s)))) {                \
		ret = device->chip->m(device, (s), &device->m);                \
		if (ret) {                                                     \
			subdev = nvkm_device_subdev(device, (s));              \
			nvkm_subdev_del(&subdev);                              \
			device->m = NULL;                                      \
			if (ret != -ENODEV) {                                  \
				nvdev_error(device, "%s ctor failed, %d\n",    \
					    nvkm_subdev_name[s], ret);         \
				goto done;                                     \
			}                                                      \
		}                                                              \
	}                                                                      \
	break
		switch (i) {
		_(NVKM_SUBDEV_BAR    ,     bar);
		_(NVKM_SUBDEV_VBIOS  ,    bios);
		_(NVKM_SUBDEV_BUS    ,     bus);
		_(NVKM_SUBDEV_CLK    ,     clk);
		_(NVKM_SUBDEV_DEVINIT, devinit);
		_(NVKM_SUBDEV_FB     ,      fb);
		_(NVKM_SUBDEV_FUSE   ,    fuse);
		_(NVKM_SUBDEV_GPIO   ,    gpio);
		_(NVKM_SUBDEV_I2C    ,     i2c);
		_(NVKM_SUBDEV_IBUS   ,    ibus);
		_(NVKM_SUBDEV_INSTMEM,    imem);
		_(NVKM_SUBDEV_LTC    ,     ltc);
		_(NVKM_SUBDEV_MC     ,      mc);
		_(NVKM_SUBDEV_MMU    ,     mmu);
		_(NVKM_SUBDEV_MXM    ,     mxm);
		_(NVKM_SUBDEV_PCI    ,     pci);
		_(NVKM_SUBDEV_PMU    ,     pmu);
		_(NVKM_SUBDEV_THERM  ,   therm);
		_(NVKM_SUBDEV_TIMER  ,   timer);
		_(NVKM_SUBDEV_VOLT   ,    volt);
		_(NVKM_ENGINE_BSP    ,     bsp);
		_(NVKM_ENGINE_CE0    ,   ce[0]);
		_(NVKM_ENGINE_CE1    ,   ce[1]);
		_(NVKM_ENGINE_CE2    ,   ce[2]);
		_(NVKM_ENGINE_CIPHER ,  cipher);
		_(NVKM_ENGINE_DISP   ,    disp);
		_(NVKM_ENGINE_DMAOBJ ,     dma);
		_(NVKM_ENGINE_FIFO   ,    fifo);
		_(NVKM_ENGINE_GR     ,      gr);
		_(NVKM_ENGINE_IFB    ,     ifb);
		_(NVKM_ENGINE_ME     ,      me);
		_(NVKM_ENGINE_MPEG   ,    mpeg);
		_(NVKM_ENGINE_MSENC  ,   msenc);
		_(NVKM_ENGINE_MSPDEC ,  mspdec);
		_(NVKM_ENGINE_MSPPP  ,   msppp);
		_(NVKM_ENGINE_MSVLD  ,   msvld);
		_(NVKM_ENGINE_PM     ,      pm);
		_(NVKM_ENGINE_SEC    ,     sec);
		_(NVKM_ENGINE_SW     ,      sw);
		_(NVKM_ENGINE_VIC    ,     vic);
		_(NVKM_ENGINE_VP     ,      vp);
		default:
			WARN_ON(1);
			continue;
		}
#undef _
	}

	ret = 0;
done:
	mutex_unlock(&nv_devices_mutex);
	return ret;
}
