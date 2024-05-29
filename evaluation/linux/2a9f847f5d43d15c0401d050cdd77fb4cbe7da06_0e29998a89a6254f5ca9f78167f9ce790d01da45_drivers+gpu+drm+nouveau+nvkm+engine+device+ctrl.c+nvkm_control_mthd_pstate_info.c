static int
nvkm_control_mthd_pstate_info(struct nvkm_control *ctrl, void *data, u32 size)
{
	union {
		struct nvif_control_pstate_info_v0 v0;
	} *args = data;
	struct nvkm_clk *clk = ctrl->device->clk;
	int ret;

	nvif_ioctl(&ctrl->object, "control pstate info size %d\n", size);
	if (nvif_unpack(args->v0, 0, 0, false)) {
		nvif_ioctl(&ctrl->object, "control pstate info vers %d\n",
			   args->v0.version);
	} else
		return ret;

	if (clk) {
		args->v0.count = clk->state_nr;
		args->v0.ustate_ac = clk->ustate_ac;
		args->v0.ustate_dc = clk->ustate_dc;
		args->v0.pwrsrc = clk->pwrsrc;
		args->v0.pstate = clk->pstate;
	} else {
		args->v0.count = 0;
		args->v0.ustate_ac = NVIF_CONTROL_PSTATE_INFO_V0_USTATE_DISABLE;
		args->v0.ustate_dc = NVIF_CONTROL_PSTATE_INFO_V0_USTATE_DISABLE;
		args->v0.pwrsrc = -ENOSYS;
		args->v0.pstate = NVIF_CONTROL_PSTATE_INFO_V0_PSTATE_UNKNOWN;
	}

	return 0;
}
