static irqreturn_t atp870u_intr_handle(int irq, void *dev_id)
{
	unsigned long flags;
	unsigned short int id;
	unsigned char i, j, c, target_id, lun,cmdp;
	unsigned char *prd;
	struct scsi_cmnd *workreq;
	unsigned long adrcnt, k;
#ifdef ED_DBGP
	unsigned long l;
#endif
	struct Scsi_Host *host = dev_id;
	struct atp_unit *dev = (struct atp_unit *)&host->hostdata;

	for (c = 0; c < 2; c++) {
		j = atp_readb_io(dev, c, 0x1f);
		if ((j & 0x80) != 0)
			break;
		dev->in_int[c] = 0;
	}
	if ((j & 0x80) == 0)
		return IRQ_NONE;
#ifdef ED_DBGP	
	printk("atp870u_intr_handle enter\n");
#endif	
	dev->in_int[c] = 1;
	cmdp = atp_readb_io(dev, c, 0x10);
	if (dev->working[c] != 0) {
		if (dev->dev_id == ATP885_DEVID) {
			if ((atp_readb_io(dev, c, 0x16) & 0x80) == 0)
				atp_writeb_io(dev, c, 0x16, (atp_readb_io(dev, c, 0x16) | 0x80));
		}		
		if ((atp_readb_pci(dev, c, 0x00) & 0x08) != 0)
		{
			for (k=0; k < 1000; k++) {
				if ((atp_readb_pci(dev, c, 2) & 0x08) == 0)
					break;
				if ((atp_readb_pci(dev, c, 2) & 0x01) == 0)
					break;
			}
		}
		atp_writeb_pci(dev, c, 0, 0x00);
		
		i = atp_readb_io(dev, c, 0x17);
		
		if (dev->dev_id == ATP885_DEVID)
			atp_writeb_pci(dev, c, 2, 0x06);

		target_id = atp_readb_io(dev, c, 0x15);

		/*
		 *	Remap wide devices onto id numbers
		 */

		if ((target_id & 0x40) != 0) {
			target_id = (target_id & 0x07) | 0x08;
		} else {
			target_id &= 0x07;
		}

		if ((j & 0x40) != 0) {
		     if (dev->last_cmd[c] == 0xff) {
			dev->last_cmd[c] = target_id;
		     }
		     dev->last_cmd[c] |= 0x40;
		}
		if (dev->dev_id == ATP885_DEVID) 
			dev->r1f[c][target_id] |= j;
#ifdef ED_DBGP
		printk("atp870u_intr_handle status = %x\n",i);
#endif	
		if (i == 0x85) {
			if ((dev->last_cmd[c] & 0xf0) != 0x40) {
			   dev->last_cmd[c] = 0xff;
			}
			if (dev->dev_id == ATP885_DEVID) {
				adrcnt = 0;
				((unsigned char *) &adrcnt)[2] = atp_readb_io(dev, c, 0x12);
				((unsigned char *) &adrcnt)[1] = atp_readb_io(dev, c, 0x13);
				((unsigned char *) &adrcnt)[0] = atp_readb_io(dev, c, 0x14);
				if (dev->id[c][target_id].last_len != adrcnt)
				{
			   		k = dev->id[c][target_id].last_len;
			   		k -= adrcnt;
			   		dev->id[c][target_id].tran_len = k;			   
			   	dev->id[c][target_id].last_len = adrcnt;			   
				}
#ifdef ED_DBGP
				printk("dev->id[c][target_id].last_len = %d dev->id[c][target_id].tran_len = %d\n",dev->id[c][target_id].last_len,dev->id[c][target_id].tran_len);
#endif		
			}

			/*
			 *      Flip wide
			 */			
			if (dev->wide_id[c] != 0) {
				atp_writeb_io(dev, c, 0x1b, 0x01);
				while ((atp_readb_io(dev, c, 0x1b) & 0x01) != 0x01)
					atp_writeb_io(dev, c, 0x1b, 0x01);
			}		
			/*
			 *	Issue more commands
			 */
			spin_lock_irqsave(dev->host->host_lock, flags);			 			 
			if (((dev->quhd[c] != dev->quend[c]) || (dev->last_cmd[c] != 0xff)) &&
			    (dev->in_snd[c] == 0)) {
#ifdef ED_DBGP
				printk("Call sent_s870\n");
#endif				
				send_s870(dev,c);
			}
			spin_unlock_irqrestore(dev->host->host_lock, flags);
			/*
			 *	Done
			 */
			dev->in_int[c] = 0;
#ifdef ED_DBGP
				printk("Status 0x85 return\n");
#endif				
			return IRQ_HANDLED;
		}

		if (i == 0x40) {
		     dev->last_cmd[c] |= 0x40;
		     dev->in_int[c] = 0;
		     return IRQ_HANDLED;
		}

		if (i == 0x21) {
			if ((dev->last_cmd[c] & 0xf0) != 0x40) {
			   dev->last_cmd[c] = 0xff;
			}
			adrcnt = 0;
			((unsigned char *) &adrcnt)[2] = atp_readb_io(dev, c, 0x12);
			((unsigned char *) &adrcnt)[1] = atp_readb_io(dev, c, 0x13);
			((unsigned char *) &adrcnt)[0] = atp_readb_io(dev, c, 0x14);
			k = dev->id[c][target_id].last_len;
			k -= adrcnt;
			dev->id[c][target_id].tran_len = k;
			dev->id[c][target_id].last_len = adrcnt;
			atp_writeb_io(dev, c, 0x10, 0x41);
			atp_writeb_io(dev, c, 0x18, 0x08);
			dev->in_int[c] = 0;
			return IRQ_HANDLED;
		}

		if (dev->dev_id == ATP885_DEVID) {
			if ((i == 0x4c) || (i == 0x4d) || (i == 0x8c) || (i == 0x8d)) {
		   		if ((i == 0x4c) || (i == 0x8c)) 
		      			i=0x48;
		   		else 
		      			i=0x49;
		   	}	
			
		}
		if ((i == 0x80) || (i == 0x8f)) {
#ifdef ED_DBGP
			printk(KERN_DEBUG "Device reselect\n");
#endif			
			lun = 0;
			if (cmdp == 0x44 || i == 0x80)
				lun = atp_readb_io(dev, c, 0x1d) & 0x07;
			else {
				if ((dev->last_cmd[c] & 0xf0) != 0x40) {
				   dev->last_cmd[c] = 0xff;
				}
				if (cmdp == 0x41) {
#ifdef ED_DBGP
					printk("cmdp = 0x41\n");
#endif						
					adrcnt = 0;
					((unsigned char *) &adrcnt)[2] = atp_readb_io(dev, c, 0x12);
					((unsigned char *) &adrcnt)[1] = atp_readb_io(dev, c, 0x13);
					((unsigned char *) &adrcnt)[0] = atp_readb_io(dev, c, 0x14);
					k = dev->id[c][target_id].last_len;
					k -= adrcnt;
					dev->id[c][target_id].tran_len = k;
					dev->id[c][target_id].last_len = adrcnt;
					atp_writeb_io(dev, c, 0x18, 0x08);
					dev->in_int[c] = 0;
					return IRQ_HANDLED;
				} else {
#ifdef ED_DBGP
					printk("cmdp != 0x41\n");
#endif						
					atp_writeb_io(dev, c, 0x10, 0x46);
					dev->id[c][target_id].dirct = 0x00;
					atp_writeb_io(dev, c, 0x12, 0x00);
					atp_writeb_io(dev, c, 0x13, 0x00);
					atp_writeb_io(dev, c, 0x14, 0x00);
					atp_writeb_io(dev, c, 0x18, 0x08);
					dev->in_int[c] = 0;
					return IRQ_HANDLED;
				}
			}
			if (dev->last_cmd[c] != 0xff) {
			   dev->last_cmd[c] |= 0x40;
			}
			if (dev->dev_id == ATP885_DEVID) {
				j = atp_readb_base(dev, 0x29) & 0xfe;
				atp_writeb_base(dev, 0x29, j);
			} else
				atp_writeb_io(dev, c, 0x10, 0x45);

			target_id = atp_readb_io(dev, c, 0x16);
			/*
			 *	Remap wide identifiers
			 */
			if ((target_id & 0x10) != 0) {
				target_id = (target_id & 0x07) | 0x08;
			} else {
				target_id &= 0x07;
			}
			if (dev->dev_id == ATP885_DEVID)
				atp_writeb_io(dev, c, 0x10, 0x45);
			workreq = dev->id[c][target_id].curr_req;
#ifdef ED_DBGP			
			scmd_printk(KERN_DEBUG, workreq, "CDB");
			for (l = 0; l < workreq->cmd_len; l++)
				printk(KERN_DEBUG " %x",workreq->cmnd[l]);
			printk("\n");
#endif	
			
			atp_writeb_io(dev, c, 0x0f, lun);
			atp_writeb_io(dev, c, 0x11, dev->id[c][target_id].devsp);
			adrcnt = dev->id[c][target_id].tran_len;
			k = dev->id[c][target_id].last_len;

			atp_writeb_io(dev, c, 0x12, ((unsigned char *) &k)[2]);
			atp_writeb_io(dev, c, 0x13, ((unsigned char *) &k)[1]);
			atp_writeb_io(dev, c, 0x14, ((unsigned char *) &k)[0]);
#ifdef ED_DBGP			
			printk("k %x, k[0] 0x%x k[1] 0x%x k[2] 0x%x\n", k, atp_readb_io(dev, c, 0x14), atp_readb_io(dev, c, 0x13), atp_readb_io(dev, c, 0x12));
#endif			
			/* Remap wide */
			j = target_id;
			if (target_id > 7) {
				j = (j & 0x07) | 0x40;
			}
			/* Add direction */
			j |= dev->id[c][target_id].dirct;
			atp_writeb_io(dev, c, 0x15, j);
			atp_writeb_io(dev, c, 0x16, 0x80);
			
			/* enable 32 bit fifo transfer */	
			if (dev->dev_id == ATP885_DEVID) {
				i = atp_readb_pci(dev, c, 1) & 0xf3;
				//j=workreq->cmnd[0];	    		    	
				if ((workreq->cmnd[0] == 0x08) || (workreq->cmnd[0] == 0x28) || (workreq->cmnd[0] == 0x0a) || (workreq->cmnd[0] == 0x2a)) {
				   i |= 0x0c;
				}
				atp_writeb_pci(dev, c, 1, i);
			} else if ((dev->dev_id == ATP880_DEVID1) ||
	    		    	   (dev->dev_id == ATP880_DEVID2) ) {
				if ((workreq->cmnd[0] == 0x08) || (workreq->cmnd[0] == 0x28) || (workreq->cmnd[0] == 0x0a) || (workreq->cmnd[0] == 0x2a))
					atp_writeb_base(dev, 0x3b, (atp_readb_base(dev, 0x3b) & 0x3f) | 0xc0);
				else
					atp_writeb_base(dev, 0x3b, atp_readb_base(dev, 0x3b) & 0x3f);
			} else {				
				if ((workreq->cmnd[0] == 0x08) || (workreq->cmnd[0] == 0x28) || (workreq->cmnd[0] == 0x0a) || (workreq->cmnd[0] == 0x2a))
					atp_writeb_base(dev, 0x3a, (atp_readb_base(dev, 0x3a) & 0xf3) | 0x08);
				else
					atp_writeb_base(dev, 0x3a, atp_readb_base(dev, 0x3a) & 0xf3);
			}	
			j = 0;
			id = 1;
			id = id << target_id;
			/*
			 *	Is this a wide device
			 */
			if ((id & dev->wide_id[c]) != 0) {
				j |= 0x01;
			}
			atp_writeb_io(dev, c, 0x1b, j);
			while ((atp_readb_io(dev, c, 0x1b) & 0x01) != j)
				atp_writeb_io(dev, c, 0x1b, j);
			if (dev->id[c][target_id].last_len == 0) {
				atp_writeb_io(dev, c, 0x18, 0x08);
				dev->in_int[c] = 0;
#ifdef ED_DBGP
				printk("dev->id[c][target_id].last_len = 0\n");
#endif					
				return IRQ_HANDLED;
			}
#ifdef ED_DBGP
			printk("target_id = %d adrcnt = %d\n",target_id,adrcnt);
#endif			
			prd = dev->id[c][target_id].prd_pos;
			while (adrcnt != 0) {
				id = ((unsigned short int *)prd)[2];
				if (id == 0) {
					k = 0x10000;
				} else {
					k = id;
				}
				if (k > adrcnt) {
					((unsigned short int *)prd)[2] = (unsigned short int)
					    (k - adrcnt);
					((unsigned long *)prd)[0] += adrcnt;
					adrcnt = 0;
					dev->id[c][target_id].prd_pos = prd;
				} else {
					adrcnt -= k;
					dev->id[c][target_id].prdaddr += 0x08;
					prd += 0x08;
					if (adrcnt == 0) {
						dev->id[c][target_id].prd_pos = prd;
					}
				}				
			}
			atp_writel_pci(dev, c, 0x04, dev->id[c][target_id].prdaddr);
#ifdef ED_DBGP
			printk("dev->id[%d][%d].prdaddr 0x%8x\n", c, target_id, dev->id[c][target_id].prdaddr);
#endif
			if (dev->dev_id != ATP885_DEVID) {
				atp_writeb_pci(dev, c, 2, 0x06);
				atp_writeb_pci(dev, c, 2, 0x00);
			}
			/*
			 *	Check transfer direction
			 */
			if (dev->id[c][target_id].dirct != 0) {
				atp_writeb_io(dev, c, 0x18, 0x08);
				atp_writeb_pci(dev, c, 0, 0x01);
				dev->in_int[c] = 0;
#ifdef ED_DBGP
				printk("status 0x80 return dirct != 0\n");
#endif				
				return IRQ_HANDLED;
			}
			atp_writeb_io(dev, c, 0x18, 0x08);
			atp_writeb_pci(dev, c, 0, 0x09);
			dev->in_int[c] = 0;
#ifdef ED_DBGP
			printk("status 0x80 return dirct = 0\n");
#endif			
			return IRQ_HANDLED;
		}

		/*
		 *	Current scsi request on this target
		 */

		workreq = dev->id[c][target_id].curr_req;

		if (i == 0x42 || i == 0x16) {
			if ((dev->last_cmd[c] & 0xf0) != 0x40) {
			   dev->last_cmd[c] = 0xff;
			}
			if (i == 0x16) {
				workreq->result = atp_readb_io(dev, c, 0x0f);
				if (((dev->r1f[c][target_id] & 0x10) != 0)&&(dev->dev_id==ATP885_DEVID)) {
					printk(KERN_WARNING "AEC67162 CRC ERROR !\n");
					workreq->result = 0x02;
				}
			} else
				workreq->result = 0x02;

			if (dev->dev_id == ATP885_DEVID) {		
				j = atp_readb_base(dev, 0x29) | 0x01;
				atp_writeb_base(dev, 0x29, j);
			}
			/*
			 *	Complete the command
			 */
			scsi_dma_unmap(workreq);

			spin_lock_irqsave(dev->host->host_lock, flags);
			(*workreq->scsi_done) (workreq);
#ifdef ED_DBGP
			   printk("workreq->scsi_done\n");
#endif	
			/*
			 *	Clear it off the queue
			 */
			dev->id[c][target_id].curr_req = NULL;
			dev->working[c]--;
			spin_unlock_irqrestore(dev->host->host_lock, flags);
			/*
			 *      Take it back wide
			 */
			if (dev->wide_id[c] != 0) {
				atp_writeb_io(dev, c, 0x1b, 0x01);
				while ((atp_readb_io(dev, c, 0x1b) & 0x01) != 0x01)
					atp_writeb_io(dev, c, 0x1b, 0x01);
			} 
			/*
			 *	If there is stuff to send and nothing going then send it
			 */
			spin_lock_irqsave(dev->host->host_lock, flags);
			if (((dev->last_cmd[c] != 0xff) || (dev->quhd[c] != dev->quend[c])) &&
			    (dev->in_snd[c] == 0)) {
#ifdef ED_DBGP
			   printk("Call sent_s870(scsi_done)\n");
#endif				   
			   send_s870(dev,c);
			}
			spin_unlock_irqrestore(dev->host->host_lock, flags);
			dev->in_int[c] = 0;
			return IRQ_HANDLED;
		}
		if ((dev->last_cmd[c] & 0xf0) != 0x40) {
		   dev->last_cmd[c] = 0xff;
		}
		if (i == 0x4f) {
			i = 0x89;
		}
		i &= 0x0f;
		if (i == 0x09) {
			atp_writel_pci(dev, c, 4, dev->id[c][target_id].prdaddr);
			atp_writeb_pci(dev, c, 2, 0x06);
			atp_writeb_pci(dev, c, 2, 0x00);
			atp_writeb_io(dev, c, 0x10, 0x41);
			if (dev->dev_id == ATP885_DEVID) {
				k = dev->id[c][target_id].last_len;
				atp_writeb_io(dev, c, 0x12, ((unsigned char *) (&k))[2]);
				atp_writeb_io(dev, c, 0x13, ((unsigned char *) (&k))[1]);
				atp_writeb_io(dev, c, 0x14, ((unsigned char *) (&k))[0]);
				dev->id[c][target_id].dirct = 0x00;
			} else {
				dev->id[c][target_id].dirct = 0x00;
			}
			atp_writeb_io(dev, c, 0x18, 0x08);
			atp_writeb_pci(dev, c, 0, 0x09);
			dev->in_int[c] = 0;
			return IRQ_HANDLED;
		}
		if (i == 0x08) {
			atp_writel_pci(dev, c, 4, dev->id[c][target_id].prdaddr);
			atp_writeb_pci(dev, c, 2, 0x06);
			atp_writeb_pci(dev, c, 2, 0x00);
			atp_writeb_io(dev, c, 0x10, 0x41);
			if (dev->dev_id == ATP885_DEVID) {		
				k = dev->id[c][target_id].last_len;
				atp_writeb_io(dev, c, 0x12, ((unsigned char *) (&k))[2]);
				atp_writeb_io(dev, c, 0x13, ((unsigned char *) (&k))[1]);
				atp_writeb_io(dev, c, 0x14, ((unsigned char *) (&k))[0]);
			}
			atp_writeb_io(dev, c, 0x15, atp_readb_io(dev, c, 0x15) | 0x20);
			dev->id[c][target_id].dirct = 0x20;
			atp_writeb_io(dev, c, 0x18, 0x08);
			atp_writeb_pci(dev, c, 0, 0x01);
			dev->in_int[c] = 0;
			return IRQ_HANDLED;
		}
		if (i == 0x0a)
			atp_writeb_io(dev, c, 0x10, 0x30);
		else
			atp_writeb_io(dev, c, 0x10, 0x46);
		dev->id[c][target_id].dirct = 0x00;
		atp_writeb_io(dev, c, 0x12, 0x00);
		atp_writeb_io(dev, c, 0x13, 0x00);
		atp_writeb_io(dev, c, 0x14, 0x00);
		atp_writeb_io(dev, c, 0x18, 0x08);
	}
	dev->in_int[c] = 0;

	return IRQ_HANDLED;
}
