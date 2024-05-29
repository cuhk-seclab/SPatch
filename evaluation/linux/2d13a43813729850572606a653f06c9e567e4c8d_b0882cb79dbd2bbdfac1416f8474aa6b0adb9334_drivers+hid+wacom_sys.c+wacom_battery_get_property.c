static int wacom_battery_get_property(struct power_supply *psy,
				      enum power_supply_property psp,
				      union power_supply_propval *val)
{
	struct wacom *wacom = container_of(psy, struct wacom, battery);
	int ret = 0;

	switch (psp) {
		case POWER_SUPPLY_PROP_SCOPE:
			val->intval = POWER_SUPPLY_SCOPE_DEVICE;
			break;
		case POWER_SUPPLY_PROP_CAPACITY:
			val->intval =
				wacom->wacom_wac.battery_capacity;
			break;
		case POWER_SUPPLY_PROP_STATUS:
			if (wacom->wacom_wac.bat_charging)
				val->intval = POWER_SUPPLY_STATUS_CHARGING;
			else if (wacom->wacom_wac.battery_capacity == 100 &&
				    wacom->wacom_wac.ps_connected)
				val->intval = POWER_SUPPLY_STATUS_FULL;
			else if (wacom->wacom_wac.ps_connected)
				val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
			else
				val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
			break;
		default:
			ret = -EINVAL;
			break;
	}

	return ret;
}
