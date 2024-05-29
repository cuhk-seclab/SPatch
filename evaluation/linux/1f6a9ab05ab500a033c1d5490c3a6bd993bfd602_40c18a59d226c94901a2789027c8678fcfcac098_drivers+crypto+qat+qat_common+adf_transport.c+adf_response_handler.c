void adf_response_handler(uintptr_t bank_addr)
{
	struct adf_etr_bank_data *bank = (void *)bank_addr;

	/* Handle all the responses and reenable IRQs */
	adf_ring_response_handler(bank);
	WRITE_CSR_INT_FLAG_AND_COL(bank->csr_addr, bank->bank_number,
				   bank->irq_mask);
}
