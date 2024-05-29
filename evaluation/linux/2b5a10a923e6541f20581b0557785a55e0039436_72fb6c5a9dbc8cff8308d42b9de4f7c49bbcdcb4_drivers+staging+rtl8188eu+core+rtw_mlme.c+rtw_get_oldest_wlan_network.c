struct	wlan_network	*rtw_get_oldest_wlan_network(struct __queue *scanned_queue)
{
	struct list_head *plist, *phead;
	struct	wlan_network	*pwlan = NULL;
	struct	wlan_network	*oldest = NULL;

	phead = get_list_head(scanned_queue);

	plist = phead->next;

		pwlan = container_of(plist, struct wlan_network, list);

		if (!pwlan->fixed) {
			if (oldest == NULL || time_after(oldest->last_scanned, pwlan->last_scanned))
				oldest = pwlan;
		}

		plist = plist->next;
	}
	return oldest;
}
