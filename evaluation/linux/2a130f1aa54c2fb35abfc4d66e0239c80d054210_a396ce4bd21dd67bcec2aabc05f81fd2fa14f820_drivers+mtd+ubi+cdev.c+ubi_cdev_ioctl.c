static long ubi_cdev_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	int err = 0;
	struct ubi_device *ubi;
	struct ubi_volume_desc *desc;
	void __user *argp = (void __user *)arg;

	if (!capable(CAP_SYS_RESOURCE))
		return -EPERM;

	ubi = ubi_get_by_major(imajor(file->f_mapping->host));
	if (!ubi)
		return -ENODEV;

	switch (cmd) {
	/* Create volume command */
	case UBI_IOCMKVOL:
	{
		struct ubi_mkvol_req req;

		dbg_gen("create volume");
		err = copy_from_user(&req, argp, sizeof(struct ubi_mkvol_req));
		if (err) {
			err = -EFAULT;
			break;
		}

		err = verify_mkvol_req(ubi, &req);
		if (err)
			break;

		mutex_lock(&ubi->device_mutex);
		err = ubi_create_volume(ubi, &req);
		mutex_unlock(&ubi->device_mutex);
		if (err)
			break;

		err = put_user(req.vol_id, (__user int32_t *)argp);
		if (err)
			err = -EFAULT;

		break;
	}

	/* Remove volume command */
	case UBI_IOCRMVOL:
	{
		int vol_id;

		dbg_gen("remove volume");
		err = get_user(vol_id, (__user int32_t *)argp);
		if (err) {
			err = -EFAULT;
			break;
		}

		desc = ubi_open_volume(ubi->ubi_num, vol_id, UBI_EXCLUSIVE);
		if (IS_ERR(desc)) {
			err = PTR_ERR(desc);
			break;
		}

		mutex_lock(&ubi->device_mutex);
		err = ubi_remove_volume(desc, 0);
		mutex_unlock(&ubi->device_mutex);

		/*
		 * The volume is deleted (unless an error occurred), and the
		 * 'struct ubi_volume' object will be freed when
		 * 'ubi_close_volume()' will call 'put_device()'.
		 */
		ubi_close_volume(desc);
		break;
	}

	/* Re-size volume command */
	case UBI_IOCRSVOL:
	{
		int pebs;
		struct ubi_rsvol_req req;

		dbg_gen("re-size volume");
		err = copy_from_user(&req, argp, sizeof(struct ubi_rsvol_req));
		if (err) {
			err = -EFAULT;
			break;
		}

		err = verify_rsvol_req(ubi, &req);
		if (err)
			break;

		desc = ubi_open_volume(ubi->ubi_num, req.vol_id, UBI_EXCLUSIVE);
		if (IS_ERR(desc)) {
			err = PTR_ERR(desc);
			break;
		}

		pebs = div_u64(req.bytes + desc->vol->usable_leb_size - 1,
			       desc->vol->usable_leb_size);

		mutex_lock(&ubi->device_mutex);
		err = ubi_resize_volume(desc, pebs);
		mutex_unlock(&ubi->device_mutex);
		ubi_close_volume(desc);
		break;
	}

	/* Re-name volumes command */
	case UBI_IOCRNVOL:
	{
		struct ubi_rnvol_req *req;

		dbg_gen("re-name volumes");
		req = kmalloc(sizeof(struct ubi_rnvol_req), GFP_KERNEL);
		if (!req) {
			err = -ENOMEM;
			break;
		}

		err = copy_from_user(req, argp, sizeof(struct ubi_rnvol_req));
		if (err) {
			err = -EFAULT;
			kfree(req);
			break;
		}

		err = rename_volumes(ubi, req);
		kfree(req);
		break;
	}

	default:
		err = -ENOTTY;
		break;
	}

	ubi_put_device(ubi);
	return err;
}
