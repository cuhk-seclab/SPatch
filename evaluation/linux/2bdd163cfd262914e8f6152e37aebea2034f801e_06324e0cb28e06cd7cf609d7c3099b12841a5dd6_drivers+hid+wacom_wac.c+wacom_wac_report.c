void wacom_wac_report(struct hid_device *hdev, struct hid_report *report)
{
	struct wacom *wacom = hid_get_drvdata(hdev);
	struct wacom_wac *wacom_wac = &wacom->wacom_wac;
	struct hid_field *field = report->field[0];

	if (wacom_wac->features.type != HID_GENERIC)
		return;

	if (WACOM_PEN_FIELD(field))
		wacom_wac_pen_pre_report(hdev, report);

	if (WACOM_FINGER_FIELD(field))
		wacom_wac_finger_pre_report(hdev, report);

	wacom_report_events(hdev, report);

	if (WACOM_PEN_FIELD(field))
		return wacom_wac_pen_report(hdev, report);

	if (WACOM_FINGER_FIELD(field))
		return wacom_wac_finger_report(hdev, report);
}
