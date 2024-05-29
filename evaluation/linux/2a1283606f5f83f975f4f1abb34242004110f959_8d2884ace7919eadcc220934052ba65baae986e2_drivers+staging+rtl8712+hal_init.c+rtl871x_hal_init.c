uint rtl871x_hal_init(struct _adapter *padapter)
{
	padapter->hw_init_completed = false;
	if (!padapter->halpriv.hal_bus_init)
		return _FAIL;
	if (padapter->halpriv.hal_bus_init(padapter) != _SUCCESS)
		return _FAIL;
	if (rtl8712_hal_init(padapter) == _SUCCESS)
		padapter->hw_init_completed = true;
	else {
		padapter->hw_init_completed = false;
		return _FAIL;
	}
	return _SUCCESS;
}
