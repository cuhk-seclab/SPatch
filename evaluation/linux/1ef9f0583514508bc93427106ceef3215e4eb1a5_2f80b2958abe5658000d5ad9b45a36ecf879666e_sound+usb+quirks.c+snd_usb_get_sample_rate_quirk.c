bool snd_usb_get_sample_rate_quirk(struct snd_usb_audio *chip)
{
	/* devices which do not support reading the sample rate. */
	switch (chip->usb_id) {
	case USB_ID(0x045E, 0x075D): /* MS Lifecam Cinema  */
	case USB_ID(0x045E, 0x076D): /* MS Lifecam HD-5000 */
	case USB_ID(0x045E, 0x0772): /* MS Lifecam Studio */
	case USB_ID(0x045E, 0x0779): /* MS Lifecam HD-3000 */
	case USB_ID(0x04D8, 0xFEEA): /* Benchmark DAC1 Pre */
	case USB_ID(0x074D, 0x3553): /* Outlaw RR2150 (Micronas UAC3553B) */
		return true;
	}
	return false;
}
