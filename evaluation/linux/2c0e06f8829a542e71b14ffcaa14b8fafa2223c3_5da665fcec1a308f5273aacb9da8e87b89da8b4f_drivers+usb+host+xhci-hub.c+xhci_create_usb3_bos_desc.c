static int xhci_create_usb3_bos_desc(struct xhci_hcd *xhci, char *buf,
				     u16 wLength)
{
	int i, ssa_count;
	u32 temp;
	u16 desc_size, ssp_cap_size, ssa_size = 0;
	bool usb3_1 = false;

	desc_size = USB_DT_BOS_SIZE + USB_DT_USB_SS_CAP_SIZE;
	ssp_cap_size = sizeof(usb_bos_descriptor) - desc_size;

	/* does xhci support USB 3.1 Enhanced SuperSpeed */
	if (xhci->usb3_rhub.min_rev >= 0x01) {
		/* does xhci provide a PSI table for SSA speed attributes? */
		if (xhci->usb3_rhub.psi_count) {
			/* two SSA entries for each unique PSI ID, RX and TX */
			ssa_count = xhci->usb3_rhub.psi_uid_count * 2;
			ssa_size = ssa_count * sizeof(u32);
			ssp_cap_size -= 16; /* skip copying the default SSA */
		}
		desc_size += ssp_cap_size;
		usb3_1 = true;
	}
	memcpy(buf, &usb_bos_descriptor, min(desc_size, wLength));

	if (usb3_1) {
		/* modify bos descriptor bNumDeviceCaps and wTotalLength */
		buf[4] += 1;
		put_unaligned_le16(desc_size + ssa_size, &buf[2]);
	}

	if (wLength < USB_DT_BOS_SIZE + USB_DT_USB_SS_CAP_SIZE)
		return wLength;

	/* Indicate whether the host has LTM support. */
	temp = readl(&xhci->cap_regs->hcc_params);
	if (HCC_LTC(temp))
		buf[8] |= USB_LTM_SUPPORT;

	/* Set the U1 and U2 exit latencies. */
	if ((xhci->quirks & XHCI_LPM_SUPPORT)) {
		temp = readl(&xhci->cap_regs->hcs_params3);
		buf[12] = HCS_U1_LATENCY(temp);
		put_unaligned_le16(HCS_U2_LATENCY(temp), &buf[13]);
	}

	/* If PSI table exists, add the custom speed attributes from it */
	if (usb3_1 && xhci->usb3_rhub.psi_count) {
		u32 ssp_cap_base, bm_attrib, psi;
		int offset;

		ssp_cap_base = USB_DT_BOS_SIZE + USB_DT_USB_SS_CAP_SIZE;

		if (wLength < desc_size)
			return wLength;
		buf[ssp_cap_base] = ssp_cap_size + ssa_size;

		/* attribute count SSAC bits 4:0 and ID count SSIC bits 8:5 */
		bm_attrib = (ssa_count - 1) & 0x1f;
		bm_attrib |= (xhci->usb3_rhub.psi_uid_count - 1) << 5;
		put_unaligned_le32(bm_attrib, &buf[ssp_cap_base + 4]);

		if (wLength < desc_size + ssa_size)
			return wLength;
		/*
		 * Create the Sublink Speed Attributes (SSA) array.
		 * The xhci PSI field and USB 3.1 SSA fields are very similar,
		 * but link type bits 7:6 differ for values 01b and 10b.
		 * xhci has also only one PSI entry for a symmetric link when
		 * USB 3.1 requires two SSA entries (RX and TX) for every link
		 */
		offset = desc_size;
		for (i = 0; i < xhci->usb3_rhub.psi_count; i++) {
			psi = xhci->usb3_rhub.psi[i];
			psi &= ~USB_SSP_SUBLINK_SPEED_RSVD;
			if ((psi & PLT_MASK) == PLT_SYM) {
			/* Symmetric, create SSA RX and TX from one PSI entry */
				put_unaligned_le32(psi, &buf[offset]);
				psi |= 1 << 7;  /* turn entry to TX */
				offset += 4;
				if (offset >= desc_size + ssa_size)
					return desc_size + ssa_size;
			} else if ((psi & PLT_MASK) == PLT_ASYM_RX) {
				/* Asymetric RX, flip bits 7:6 for SSA */
				psi ^= PLT_MASK;
			}
			put_unaligned_le32(psi, &buf[offset]);
			offset += 4;
			if (offset >= desc_size + ssa_size)
				return desc_size + ssa_size;
		}
	}
	/* ssa_size is 0 for other than usb 3.1 hosts */
	return desc_size + ssa_size;
}
