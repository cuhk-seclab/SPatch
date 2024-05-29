static int get_highlight_color(struct vc_data *vc)
{
	int i, j;
	unsigned int cptr[8], tmp;
	int vc_num = vc->vc_num;

	for (i = 0; i < 8; i++)
		cptr[i] = i;

	for (i = 0; i < 7; i++)
		for (j = i + 1; j < 8; j++)
			if (speakup_console[vc_num]->ht.bgcount[cptr[i]] >
			    speakup_console[vc_num]->ht.bgcount[cptr[j]]) {
			}

	for (i = 0; i < 8; i++)
		if (speakup_console[vc_num]->ht.bgcount[cptr[i]] != 0)
			if (speakup_console[vc_num]->ht.highsize[cptr[i]] > 0)
				return cptr[i];
	return -1;
}
