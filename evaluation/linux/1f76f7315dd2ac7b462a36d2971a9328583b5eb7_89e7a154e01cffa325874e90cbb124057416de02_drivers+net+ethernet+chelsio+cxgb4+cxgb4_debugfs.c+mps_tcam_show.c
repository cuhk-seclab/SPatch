static int mps_tcam_show(struct seq_file *seq, void *v)
{
	struct adapter *adap = seq->private;
	unsigned int chip_ver = CHELSIO_CHIP_VERSION(adap->params.chip);
	if (v == SEQ_START_TOKEN) {
		if (chip_ver > CHELSIO_T5) {
			seq_puts(seq, "Idx  Ethernet address     Mask     "
				 "  VNI   Mask   IVLAN Vld "
				 "DIP_Hit   Lookup  Port "
				 "Vld Ports PF  VF                           "
				 "Replication                                "
				 "    P0 P1 P2 P3  ML\n");
		} else {
			if (adap->params.arch.mps_rplc_size > 128)
				seq_puts(seq, "Idx  Ethernet address     Mask     "
					 "Vld Ports PF  VF                           "
					 "Replication                                "
					 "    P0 P1 P2 P3  ML\n");
			else
				seq_puts(seq, "Idx  Ethernet address     Mask     "
					 "Vld Ports PF  VF              Replication"
					 "	         P0 P1 P2 P3  ML\n");
		}
	} else {
		u64 mask;
		u8 addr[ETH_ALEN];
		bool replicate, dip_hit = false, vlan_vld = false;
		unsigned int idx = (uintptr_t)v - 2;
		u64 tcamy, tcamx, val;
		u32 cls_lo, cls_hi, ctl, data2, vnix = 0, vniy = 0;
		u32 rplc[8] = {0};
		u8 lookup_type = 0, port_num = 0;
		u16 ivlan = 0;

		if (chip_ver > CHELSIO_T5) {
			/* CtlCmdType - 0: Read, 1: Write
			 * CtlTcamSel - 0: TCAM0, 1: TCAM1
			 * CtlXYBitSel- 0: Y bit, 1: X bit
			 */

			/* Read tcamy */
			ctl = CTLCMDTYPE_V(0) | CTLXYBITSEL_V(0);
			if (idx < 256)
				ctl |= CTLTCAMINDEX_V(idx) | CTLTCAMSEL_V(0);
			else
				ctl |= CTLTCAMINDEX_V(idx - 256) |
				       CTLTCAMSEL_V(1);
			t4_write_reg(adap, MPS_CLS_TCAM_DATA2_CTL_A, ctl);
			val = t4_read_reg(adap, MPS_CLS_TCAM_DATA1_A);
			tcamy = DMACH_G(val) << 32;
			tcamy |= t4_read_reg(adap, MPS_CLS_TCAM_DATA0_A);
			data2 = t4_read_reg(adap, MPS_CLS_TCAM_DATA2_CTL_A);
			lookup_type = DATALKPTYPE_G(data2);
			/* 0 - Outer header, 1 - Inner header
			 * [71:48] bit locations are overloaded for
			 * outer vs. inner lookup types.
			 */
			if (lookup_type && (lookup_type != DATALKPTYPE_M)) {
				/* Inner header VNI */
				vniy = ((data2 & DATAVIDH2_F) << 23) |
				       (DATAVIDH1_G(data2) << 16) | VIDL_G(val);
				dip_hit = data2 & DATADIPHIT_F;
			} else {
				vlan_vld = data2 & DATAVIDH2_F;
				ivlan = VIDL_G(val);
			}
			port_num = DATAPORTNUM_G(data2);

			/* Read tcamx. Change the control param */
			ctl |= CTLXYBITSEL_V(1);
			t4_write_reg(adap, MPS_CLS_TCAM_DATA2_CTL_A, ctl);
			val = t4_read_reg(adap, MPS_CLS_TCAM_DATA1_A);
			tcamx = DMACH_G(val) << 32;
			tcamx |= t4_read_reg(adap, MPS_CLS_TCAM_DATA0_A);
			data2 = t4_read_reg(adap, MPS_CLS_TCAM_DATA2_CTL_A);
			if (lookup_type && (lookup_type != DATALKPTYPE_M)) {
				/* Inner header VNI mask */
				vnix = ((data2 & DATAVIDH2_F) << 23) |
				       (DATAVIDH1_G(data2) << 16) | VIDL_G(val);
			}
		} else {
			tcamy = t4_read_reg64(adap, MPS_CLS_TCAM_Y_L(idx));
			tcamx = t4_read_reg64(adap, MPS_CLS_TCAM_X_L(idx));
		}

		cls_lo = t4_read_reg(adap, MPS_CLS_SRAM_L(idx));
		cls_hi = t4_read_reg(adap, MPS_CLS_SRAM_H(idx));

		if (tcamx & tcamy) {
			seq_printf(seq, "%3u         -\n", idx);
			goto out;
		}

		rplc[0] = rplc[1] = rplc[2] = rplc[3] = 0;
		if (chip_ver > CHELSIO_T5)
			replicate = (cls_lo & T6_REPLICATE_F);
		else
			replicate = (cls_lo & REPLICATE_F);

		if (replicate) {
			struct fw_ldst_cmd ldst_cmd;
			int ret;
			struct fw_ldst_mps_rplc mps_rplc;
			u32 ldst_addrspc;

			memset(&ldst_cmd, 0, sizeof(ldst_cmd));
			ldst_addrspc =
				FW_LDST_CMD_ADDRSPACE_V(FW_LDST_ADDRSPC_MPS);
			ldst_cmd.op_to_addrspace =
				htonl(FW_CMD_OP_V(FW_LDST_CMD) |
				      FW_CMD_REQUEST_F |
				      FW_CMD_READ_F |
				      ldst_addrspc);
			ldst_cmd.cycles_to_len16 = htonl(FW_LEN16(ldst_cmd));
			ldst_cmd.u.mps.rplc.fid_idx =
				htons(FW_LDST_CMD_FID_V(FW_LDST_MPS_RPLC) |
				      FW_LDST_CMD_IDX_V(idx));
			ret = t4_wr_mbox(adap, adap->mbox, &ldst_cmd,
					 sizeof(ldst_cmd), &ldst_cmd);
			if (ret)
				dev_warn(adap->pdev_dev, "Can't read MPS "
					 "replication map for idx %d: %d\n",
					 idx, -ret);
			else {
				mps_rplc = ldst_cmd.u.mps.rplc;
				rplc[0] = ntohl(mps_rplc.rplc31_0);
				rplc[1] = ntohl(mps_rplc.rplc63_32);
				rplc[2] = ntohl(mps_rplc.rplc95_64);
				rplc[3] = ntohl(mps_rplc.rplc127_96);
				if (adap->params.arch.mps_rplc_size > 128) {
					rplc[4] = ntohl(mps_rplc.rplc159_128);
					rplc[5] = ntohl(mps_rplc.rplc191_160);
					rplc[6] = ntohl(mps_rplc.rplc223_192);
					rplc[7] = ntohl(mps_rplc.rplc255_224);
				}
			}
		}

		tcamxy2valmask(tcamx, tcamy, addr, &mask);
		if (chip_ver > CHELSIO_T5) {
			/* Inner header lookup */
			if (lookup_type && (lookup_type != DATALKPTYPE_M)) {
				seq_printf(seq,
					   "%3u %02x:%02x:%02x:%02x:%02x:%02x "
					   "%012llx %06x %06x    -    -   %3c"
					   "      'I'  %4x   "
					   "%3c   %#x%4u%4d", idx, addr[0],
					   addr[1], addr[2], addr[3],
					   addr[4], addr[5],
					   (unsigned long long)mask,
					   vniy, vnix, dip_hit ? 'Y' : 'N',
					   port_num,
					   (cls_lo & T6_SRAM_VLD_F) ? 'Y' : 'N',
					   PORTMAP_G(cls_hi),
					   T6_PF_G(cls_lo),
					   (cls_lo & T6_VF_VALID_F) ?
					   T6_VF_G(cls_lo) : -1);
			} else {
				seq_printf(seq,
					   "%3u %02x:%02x:%02x:%02x:%02x:%02x "
					   "%012llx    -       -   ",
					   idx, addr[0], addr[1], addr[2],
					   addr[3], addr[4], addr[5],
					   (unsigned long long)mask);

				if (vlan_vld)
					seq_printf(seq, "%4u   Y     ", ivlan);
				else
					seq_puts(seq, "  -    N     ");

				seq_printf(seq,
					   "-      %3c  %4x   %3c   %#x%4u%4d",
					   lookup_type ? 'I' : 'O', port_num,
					   (cls_lo & T6_SRAM_VLD_F) ? 'Y' : 'N',
					   PORTMAP_G(cls_hi),
					   T6_PF_G(cls_lo),
					   (cls_lo & T6_VF_VALID_F) ?
					   T6_VF_G(cls_lo) : -1);
			}
		} else
			seq_printf(seq, "%3u %02x:%02x:%02x:%02x:%02x:%02x "
				   "%012llx%3c   %#x%4u%4d",
				   idx, addr[0], addr[1], addr[2], addr[3],
				   addr[4], addr[5], (unsigned long long)mask,
				   (cls_lo & SRAM_VLD_F) ? 'Y' : 'N',
				   PORTMAP_G(cls_hi),
				   PF_G(cls_lo),
				   (cls_lo & VF_VALID_F) ? VF_G(cls_lo) : -1);

		if (replicate) {
			if (adap->params.arch.mps_rplc_size > 128)
				seq_printf(seq, " %08x %08x %08x %08x "
					   "%08x %08x %08x %08x",
					   rplc[7], rplc[6], rplc[5], rplc[4],
					   rplc[3], rplc[2], rplc[1], rplc[0]);
			else
				seq_printf(seq, " %08x %08x %08x %08x",
					   rplc[3], rplc[2], rplc[1], rplc[0]);
		} else {
			if (adap->params.arch.mps_rplc_size > 128)
				seq_printf(seq, "%72c", ' ');
			else
				seq_printf(seq, "%36c", ' ');
		}

		if (chip_ver > CHELSIO_T5)
			seq_printf(seq, "%4u%3u%3u%3u %#x\n",
				   T6_SRAM_PRIO0_G(cls_lo),
				   T6_SRAM_PRIO1_G(cls_lo),
				   T6_SRAM_PRIO2_G(cls_lo),
				   T6_SRAM_PRIO3_G(cls_lo),
				   (cls_lo >> T6_MULTILISTEN0_S) & 0xf);
		else
			seq_printf(seq, "%4u%3u%3u%3u %#x\n",
				   SRAM_PRIO0_G(cls_lo), SRAM_PRIO1_G(cls_lo),
				   SRAM_PRIO2_G(cls_lo), SRAM_PRIO3_G(cls_lo),
				   (cls_lo >> MULTILISTEN0_S) & 0xf);
	}
out:	return 0;
}
