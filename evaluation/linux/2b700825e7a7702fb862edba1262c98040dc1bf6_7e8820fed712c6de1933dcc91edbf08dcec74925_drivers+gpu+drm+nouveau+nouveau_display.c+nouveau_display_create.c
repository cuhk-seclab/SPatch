int
nouveau_display_create(struct drm_device *dev)
{
	struct nouveau_drm *drm = nouveau_drm(dev);
	struct nvkm_device *device = nvxx_device(&drm->device);
	struct nouveau_display *disp;
	int ret;

	disp = drm->display = kzalloc(sizeof(*disp), GFP_KERNEL);
	if (!disp)
		return -ENOMEM;

	drm_mode_config_init(dev);
	drm_mode_create_scaling_mode_property(dev);
	drm_mode_create_dvi_i_properties(dev);

	dev->mode_config.funcs = &nouveau_mode_config_funcs;
	dev->mode_config.fb_base = device->func->resource_addr(device, 1);

	dev->mode_config.min_width = 0;
	dev->mode_config.min_height = 0;
	if (drm->device.info.family < NV_DEVICE_INFO_V0_CELSIUS) {
		dev->mode_config.max_width = 2048;
		dev->mode_config.max_height = 2048;
	} else
	if (drm->device.info.family < NV_DEVICE_INFO_V0_TESLA) {
		dev->mode_config.max_width = 4096;
		dev->mode_config.max_height = 4096;
	} else {
		dev->mode_config.max_width = 8192;
		dev->mode_config.max_height = 8192;
	}

	dev->mode_config.preferred_depth = 24;
	dev->mode_config.prefer_shadow = 1;

	if (drm->device.info.chipset < 0x11)
		dev->mode_config.async_page_flip = false;
	else
		dev->mode_config.async_page_flip = true;

	drm_kms_helper_poll_init(dev);
	drm_kms_helper_poll_disable(dev);

	if (nouveau_modeset != 2 && drm->vbios.dcb.entries) {
		static const u16 oclass[] = {
			GM204_DISP,
			GM107_DISP,
			GK110_DISP,
			GK104_DISP,
			GF110_DISP,
			GT214_DISP,
			GT206_DISP,
			GT200_DISP,
			G82_DISP,
			NV50_DISP,
			NV04_DISP,
		};
		int i;

		for (i = 0, ret = -ENODEV; ret && i < ARRAY_SIZE(oclass); i++) {
			ret = nvif_object_init(&drm->device.object,
					       NVDRM_DISPLAY, oclass[i],
					       NULL, 0, &disp->disp);
		}

		if (ret == 0) {
			nouveau_display_create_properties(dev);
			if (disp->disp.oclass < NV50_DISP)
				ret = nv04_display_create(dev);
			else
				ret = nv50_display_create(dev);
		}
	} else {
		ret = 0;
	}

	if (ret)
		goto disp_create_err;

	if (dev->mode_config.num_crtc) {
		ret = nouveau_display_vblank_init(dev);
		if (ret)
			goto vblank_err;
	}

	nouveau_backlight_init(dev);
	return 0;

vblank_err:
	disp->dtor(dev);
disp_create_err:
	drm_kms_helper_poll_fini(dev);
	drm_mode_config_cleanup(dev);
	return ret;
}
