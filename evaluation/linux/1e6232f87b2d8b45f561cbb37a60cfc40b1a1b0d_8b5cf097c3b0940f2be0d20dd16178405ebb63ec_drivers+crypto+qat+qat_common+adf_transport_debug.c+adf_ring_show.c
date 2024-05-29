static int adf_ring_show(struct seq_file *sfile, void *v)
{
	struct adf_etr_ring_data *ring = sfile->private;
	struct adf_etr_bank_data *bank = ring->bank;
	uint32_t *msg = v;
	void __iomem *csr = ring->bank->csr_addr;
	int i, x;

	if (v == SEQ_START_TOKEN) {
		int head, tail, empty;

		head = READ_CSR_RING_HEAD(csr, bank->bank_number,
					  ring->ring_number);
		tail = READ_CSR_RING_TAIL(csr, bank->bank_number,
					  ring->ring_number);
		empty = READ_CSR_E_STAT(csr, bank->bank_number);

		seq_puts(sfile, "------- Ring configuration -------\n");
		seq_printf(sfile, "ring name: %s\n",
			   ring->ring_debug->ring_name);
		seq_printf(sfile, "ring num %d, bank num %d\n",
			   ring->ring_number, ring->bank->bank_number);
		seq_printf(sfile, "head %x, tail %x, empty: %d\n",
			   head, tail, (empty & 1 << ring->ring_number)
			   >> ring->ring_number);
		seq_printf(sfile, "ring size %d, msg size %d\n",
			   ADF_SIZE_TO_RING_SIZE_IN_BYTES(ring->ring_size),
			   ADF_MSG_SIZE_TO_BYTES(ring->msg_size));
		seq_puts(sfile, "----------- Ring data ------------\n");
		return 0;
	}
	seq_printf(sfile, "%p:", msg);
	x = 0;
	i = 0;
	for (; i < (ADF_MSG_SIZE_TO_BYTES(ring->msg_size) >> 2); i++) {
		seq_printf(sfile, " %08X", *(msg + i));
		if ((ADF_MSG_SIZE_TO_BYTES(ring->msg_size) >> 2) != i + 1 &&
		    (++x == 8)) {
			seq_printf(sfile, "\n%p:", msg + i + 1);
			x = 0;
		}
	}
	seq_puts(sfile, "\n");
	return 0;
}
