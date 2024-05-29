int
scsih_qcmd(struct Scsi_Host *shost, struct scsi_cmnd *scmd)
{
	struct MPT3SAS_ADAPTER *ioc = shost_priv(shost);
	struct MPT3SAS_DEVICE *sas_device_priv_data;
	struct MPT3SAS_TARGET *sas_target_priv_data;
	struct _raid_device *raid_device;
	Mpi2SCSIIORequest_t *mpi_request;
	u32 mpi_control;
	u16 smid;
	u16 handle;

	if (ioc->logging_level & MPT_DEBUG_SCSI)
		scsi_print_command(scmd);

	sas_device_priv_data = scmd->device->hostdata;
	if (!sas_device_priv_data || !sas_device_priv_data->sas_target) {
		scmd->result = DID_NO_CONNECT << 16;
		scmd->scsi_done(scmd);
		return 0;
	}

	if (ioc->pci_error_recovery || ioc->remove_host) {
		scmd->result = DID_NO_CONNECT << 16;
		scmd->scsi_done(scmd);
		return 0;
	}

	sas_target_priv_data = sas_device_priv_data->sas_target;

	/* invalid device handle */
	handle = sas_target_priv_data->handle;
	if (handle == MPT3SAS_INVALID_DEVICE_HANDLE) {
		scmd->result = DID_NO_CONNECT << 16;
		scmd->scsi_done(scmd);
		return 0;
	}


	/* host recovery or link resets sent via IOCTLs */
	if (ioc->shost_recovery || ioc->ioc_link_reset_in_progress)
		return SCSI_MLQUEUE_HOST_BUSY;

	/* device has been deleted */
	else if (sas_target_priv_data->deleted) {
		scmd->result = DID_NO_CONNECT << 16;
		scmd->scsi_done(scmd);
		return 0;
	/* device busy with task managment */
	} else if (sas_target_priv_data->tm_busy ||
	    sas_device_priv_data->block)
		return SCSI_MLQUEUE_DEVICE_BUSY;

	if (scmd->sc_data_direction == DMA_FROM_DEVICE)
		mpi_control = MPI2_SCSIIO_CONTROL_READ;
	else if (scmd->sc_data_direction == DMA_TO_DEVICE)
		mpi_control = MPI2_SCSIIO_CONTROL_WRITE;
	else
		mpi_control = MPI2_SCSIIO_CONTROL_NODATATRANSFER;

	/* set tags */
	mpi_control |= MPI2_SCSIIO_CONTROL_SIMPLEQ;

	/* Make sure Device is not raid volume.
	 * We do not expose raid functionality to upper layer for warpdrive.
	 */
	if (!ioc->is_warpdrive && !scsih_is_raid(&scmd->device->sdev_gendev)
	    && sas_is_tlr_enabled(scmd->device) && scmd->cmd_len != 32)
		mpi_control |= MPI2_SCSIIO_CONTROL_TLR_ON;

	smid = mpt3sas_base_get_smid_scsiio(ioc, ioc->scsi_io_cb_idx, scmd);
	if (!smid) {
		pr_err(MPT3SAS_FMT "%s: failed obtaining a smid\n",
		    ioc->name, __func__);
		goto out;
	}
	mpi_request = mpt3sas_base_get_msg_frame(ioc, smid);
	memset(mpi_request, 0, sizeof(Mpi2SCSIIORequest_t));
	_scsih_setup_eedp(ioc, scmd, mpi_request);

	if (scmd->cmd_len == 32)
		mpi_control |= 4 << MPI2_SCSIIO_CONTROL_ADDCDBLEN_SHIFT;
	mpi_request->Function = MPI2_FUNCTION_SCSI_IO_REQUEST;
	if (sas_device_priv_data->sas_target->flags &
	    MPT_TARGET_FLAGS_RAID_COMPONENT)
		mpi_request->Function = MPI2_FUNCTION_RAID_SCSI_IO_PASSTHROUGH;
	else
		mpi_request->Function = MPI2_FUNCTION_SCSI_IO_REQUEST;
	mpi_request->DevHandle = cpu_to_le16(handle);
	mpi_request->DataLength = cpu_to_le32(scsi_bufflen(scmd));
	mpi_request->Control = cpu_to_le32(mpi_control);
	mpi_request->IoFlags = cpu_to_le16(scmd->cmd_len);
	mpi_request->MsgFlags = MPI2_SCSIIO_MSGFLAGS_SYSTEM_SENSE_ADDR;
	mpi_request->SenseBufferLength = SCSI_SENSE_BUFFERSIZE;
	mpi_request->SenseBufferLowAddress =
	    mpt3sas_base_get_sense_buffer_dma(ioc, smid);
	mpi_request->SGLOffset0 = offsetof(Mpi2SCSIIORequest_t, SGL) / 4;
	int_to_scsilun(sas_device_priv_data->lun, (struct scsi_lun *)
	    mpi_request->LUN);
	memcpy(mpi_request->CDB.CDB32, scmd->cmnd, scmd->cmd_len);

	if (mpi_request->DataLength) {
		if (ioc->build_sg_scmd(ioc, scmd, smid)) {
			mpt3sas_base_free_smid(ioc, smid);
			goto out;
		}
	} else
		ioc->build_zero_len_sge(ioc, &mpi_request->SGL);

	raid_device = sas_target_priv_data->raid_device;
	if (raid_device && raid_device->direct_io_enabled)
		mpt3sas_setup_direct_io(ioc, scmd, raid_device, mpi_request,
		    smid);

	if (likely(mpi_request->Function == MPI2_FUNCTION_SCSI_IO_REQUEST)) {
		if (sas_target_priv_data->flags & MPT_TARGET_FASTPATH_IO) {
			mpi_request->IoFlags = cpu_to_le16(scmd->cmd_len |
			    MPI25_SCSIIO_IOFLAGS_FAST_PATH);
			mpt3sas_base_put_smid_fast_path(ioc, smid, handle);
		} else
			mpt3sas_base_put_smid_scsi_io(ioc, smid,
			    le16_to_cpu(mpi_request->DevHandle));
	} else
		mpt3sas_base_put_smid_default(ioc, smid);
	return 0;

 out:
	return SCSI_MLQUEUE_HOST_BUSY;
}
