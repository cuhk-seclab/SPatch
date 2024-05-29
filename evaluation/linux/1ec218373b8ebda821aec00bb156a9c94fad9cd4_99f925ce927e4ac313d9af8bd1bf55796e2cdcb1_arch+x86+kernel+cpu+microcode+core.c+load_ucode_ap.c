void load_ucode_ap(void)
{
	int vendor, family;

	if (check_loader_disabled_ap())
		return;

	if (!have_cpuid_p())
		return;

	vendor = x86_cpuid_vendor();
	family = x86_cpuid_family();

	switch (vendor) {
	case X86_VENDOR_INTEL:
		if (family >= 6)
			load_ucode_intel_ap();
		break;
	case X86_VENDOR_AMD:
		if (family >= 0x10)
			load_ucode_amd_ap();
		break;
	default:
		break;
	}
}
