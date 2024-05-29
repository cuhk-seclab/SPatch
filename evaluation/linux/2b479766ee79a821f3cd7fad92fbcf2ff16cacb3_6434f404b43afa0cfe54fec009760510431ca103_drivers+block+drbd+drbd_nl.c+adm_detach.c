static int adm_detach(struct drbd_device *device, int force)
{
	enum drbd_state_rv retcode;
	void *buffer;
	int ret;

	if (force) {
		set_bit(FORCE_DETACH, &device->flags);
		drbd_force_state(device, NS(disk, D_FAILED));
		retcode = SS_SUCCESS;
		goto out;
	}

	drbd_suspend_io(device); /* so no-one is stuck in drbd_al_begin_io */
	buffer = drbd_md_get_buffer(device, __func__); /* make sure there is no in-flight meta-data IO */
	if (buffer) {
		retcode = drbd_request_state(device, NS(disk, D_FAILED));
		drbd_md_put_buffer(device);
	} else /* already <= D_FAILED */
		retcode = SS_NOTHING_TO_DO;
	/* D_FAILED will transition to DISKLESS. */
	drbd_resume_io(device);
	ret = wait_event_interruptible(device->misc_wait,
			device->state.disk != D_FAILED);
	if ((int)retcode == (int)SS_IS_DISKLESS)
		retcode = SS_NOTHING_TO_DO;
	if (ret)
		retcode = ERR_INTR;
out:
	return retcode;
}
