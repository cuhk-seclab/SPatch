static int verify_pmtmr_rate(void)
{
	cycle_t value1, value2;
	unsigned long count, delta;

	mach_prepare_counter();
	value1 = clocksource_acpi_pm.read(&clocksource_acpi_pm);
	mach_countup(&count);
	value2 = clocksource_acpi_pm.read(&clocksource_acpi_pm);
	delta = (value2 - value1) & ACPI_PM_MASK;

	/* Check that the PMTMR delta is within 5% of what we expect */
	if (delta < (PMTMR_EXPECTED_RATE * 19) / 20 ||
	    delta > (PMTMR_EXPECTED_RATE * 21) / 20) {
		pr_info("PM-Timer running at invalid rate: %lu%% of normal - aborting.\n",
			100UL * delta / PMTMR_EXPECTED_RATE);
		return -1;
	}

	return 0;
}
