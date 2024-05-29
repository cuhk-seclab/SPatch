static void
nv44_vm_unmap(struct nvkm_gpuobj *pgt, u32 pte, u32 cnt)
{
	struct nv04_mmu *mmu = (void *)nvkm_mmu(pgt);

	if (pte & 3) {
		u32  max = 4 - (pte & 3);
		u32 part = (cnt > max) ? max : cnt;
		nv44_vm_fill(pgt, mmu->null, NULL, pte, part);
		pte  += part;
		cnt  -= part;
	}

	while (cnt >= 4) {
		nv_wo32(pgt, pte++ * 4, 0x00000000);
		nv_wo32(pgt, pte++ * 4, 0x00000000);
		nv_wo32(pgt, pte++ * 4, 0x00000000);
		nv_wo32(pgt, pte++ * 4, 0x00000000);
		cnt -= 4;
	}

	if (cnt)
		nv44_vm_fill(pgt, mmu->null, NULL, pte, cnt);
}
