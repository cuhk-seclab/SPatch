static int compat_i915_getparam(struct file *file, unsigned int cmd,
				unsigned long arg)
{
	struct drm_i915_getparam32 req32;
	drm_i915_getparam_t __user *request;

	if (copy_from_user(&req32, (void __user *)arg, sizeof(req32)))
		return -EFAULT;

	request = compat_alloc_user_space(sizeof(*request));
	if (!access_ok(VERIFY_WRITE, request, sizeof(*request))
	    || __put_user(req32.param, &request->param)
	    || __put_user((void __user *)(unsigned long)req32.value,
			  &request->value))
		return -EFAULT;

	return drm_ioctl(file, DRM_IOCTL_I915_GETPARAM,
			 (unsigned long)request);
}
