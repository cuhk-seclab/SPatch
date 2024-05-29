static void
g84_cipher_intr(struct nvkm_subdev *subdev)
{
	struct nvkm_fifo *fifo = nvkm_fifo(subdev);
	struct nvkm_engine *engine = nv_engine(subdev);
	struct nvkm_object *engctx;
	struct nvkm_engine *cipher = (void *)subdev;
	struct nvkm_device *device = cipher->subdev.device;
	u32 stat = nvkm_rd32(device, 0x102130);
	u32 mthd = nvkm_rd32(device, 0x102190);
	u32 data = nvkm_rd32(device, 0x102194);
	u32 inst = nvkm_rd32(device, 0x102188) & 0x7fffffff;
	char msg[128];
	int chid;

	engctx = nvkm_engctx_get(engine, inst);
	chid   = fifo->chid(fifo, engctx);

	if (stat) {
		nvkm_snprintbf(msg, sizeof(msg), g84_cipher_intr_mask, stat);
		nvkm_error(subdev,  "%08x [%s] ch %d [%010llx %s] "
				    "mthd %04x data %08x\n",
			   stat, msg, chid, (u64)inst << 12,
			   nvkm_client_name(engctx), mthd, data);
	}

	nvkm_wr32(device, 0x102130, stat);
	nvkm_wr32(device, 0x10200c, 0x10);

	nvkm_engctx_put(engctx);
}
