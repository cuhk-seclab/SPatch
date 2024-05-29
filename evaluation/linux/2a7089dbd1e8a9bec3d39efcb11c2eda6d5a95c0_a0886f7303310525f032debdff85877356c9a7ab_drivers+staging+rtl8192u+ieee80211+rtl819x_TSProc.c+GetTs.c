bool GetTs(
	struct ieee80211_device		*ieee,
	PTS_COMMON_INFO			*ppTS,
	u8				*Addr,
	u8				TID,
	TR_SELECT			TxRxSelect,  //Rx:1, Tx:0
	bool				bAddNewTs
	)
{
	u8	UP = 0;
	//
	// We do not build any TS for Broadcast or Multicast stream.
	// So reject these kinds of search here.
	//
	if (is_multicast_ether_addr(Addr))
	{
		IEEE80211_DEBUG(IEEE80211_DL_ERR, "get TS for Broadcast or Multicast\n");
		return false;
	}

	if (ieee->current_network.qos_data.supported == 0)
		UP = 0;
	else
	{
		// In WMM case: we use 4 TID only
		if (!IsACValid(TID))
		{
			IEEE80211_DEBUG(IEEE80211_DL_ERR, " in %s(), TID(%d) is not valid\n", __func__, TID);
			return false;
		}

		switch (TID)
		{
		case 0:
		case 3:
			UP = 0;
			break;

		case 1:
		case 2:
			UP = 2;
			break;

		case 4:
		case 5:
			UP = 5;
			break;

		case 6:
		case 7:
			UP = 7;
			break;
		}
	}

	*ppTS = SearchAdmitTRStream(
			ieee,
			Addr,
			UP,
			TxRxSelect);
	if(*ppTS != NULL)
	{
		return true;
	}
	else
	{
		if (!bAddNewTs) {
			IEEE80211_DEBUG(IEEE80211_DL_TS, "add new TS failed(tid:%d)\n", UP);
			return false;
		}
		else
		{
			//
			// Create a new Traffic stream for current Tx/Rx
			// This is for EDCA and WMM to add a new TS.
			// For HCCA or WMMSA, TS cannot be addmit without negotiation.
			//
			TSPEC_BODY	TSpec;
			PQOS_TSINFO		pTSInfo = &TSpec.f.TSInfo;
			struct list_head	*pUnusedList =
								(TxRxSelect == TX_DIR)?
								(&ieee->Tx_TS_Unused_List):
								(&ieee->Rx_TS_Unused_List);

			struct list_head	*pAddmitList =
								(TxRxSelect == TX_DIR)?
								(&ieee->Tx_TS_Admit_List):
								(&ieee->Rx_TS_Admit_List);

			DIRECTION_VALUE		Dir =		(ieee->iw_mode == IW_MODE_MASTER)?
								((TxRxSelect==TX_DIR)?DIR_DOWN:DIR_UP):
								((TxRxSelect==TX_DIR)?DIR_UP:DIR_DOWN);
			IEEE80211_DEBUG(IEEE80211_DL_TS, "to add Ts\n");
			if(!list_empty(pUnusedList))
			{
				(*ppTS) = list_entry(pUnusedList->next, TS_COMMON_INFO, List);
				list_del_init(&(*ppTS)->List);
				if(TxRxSelect==TX_DIR)
				{
					PTX_TS_RECORD tmp = container_of(*ppTS, TX_TS_RECORD, TsCommonInfo);
					ResetTxTsEntry(tmp);
				}
				else{
					PRX_TS_RECORD tmp = container_of(*ppTS, RX_TS_RECORD, TsCommonInfo);
					ResetRxTsEntry(tmp);
				}

				IEEE80211_DEBUG(IEEE80211_DL_TS, "to init current TS, UP:%d, Dir:%d, addr:%pM\n", UP, Dir, Addr);
				// Prepare TS Info releated field
				pTSInfo->field.ucTrafficType = 0;			// Traffic type: WMM is reserved in this field
				pTSInfo->field.ucTSID = UP;			// TSID
				pTSInfo->field.ucDirection = Dir;			// Direction: if there is DirectLink, this need additional consideration.
				pTSInfo->field.ucAccessPolicy = 1;		// Access policy
				pTSInfo->field.ucAggregation = 0;		// Aggregation
				pTSInfo->field.ucPSB = 0;				// Aggregation
				pTSInfo->field.ucUP = UP;				// User priority
				pTSInfo->field.ucTSInfoAckPolicy = 0;		// Ack policy
				pTSInfo->field.ucSchedule = 0;			// Schedule

				MakeTSEntry(*ppTS, Addr, &TSpec, NULL, 0, 0);
				AdmitTS(ieee, *ppTS, 0);
				list_add_tail(&((*ppTS)->List), pAddmitList);
				// if there is DirectLink, we need to do additional operation here!!

				return true;
			}
			else
			{
				IEEE80211_DEBUG(IEEE80211_DL_ERR, "in function %s() There is not enough TS record to be used!!", __func__);
				return false;
			}
		}
	}
}
