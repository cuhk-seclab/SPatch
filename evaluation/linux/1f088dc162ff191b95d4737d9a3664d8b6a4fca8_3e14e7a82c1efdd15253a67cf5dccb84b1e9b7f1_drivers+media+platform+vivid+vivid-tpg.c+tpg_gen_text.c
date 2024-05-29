void tpg_gen_text(struct tpg_data *tpg, u8 *basep[TPG_MAX_PLANES][2],
		int y, int x, char *text)
{
	int line;
	unsigned step = V4L2_FIELD_HAS_T_OR_B(tpg->field) ? 2 : 1;
	unsigned div = step;
	unsigned first = 0;
	unsigned len = strlen(text);
	unsigned p;

	if (font8x16 == NULL || basep == NULL)
		return;

	/* Checks if it is possible to show string */
	if (y + 16 >= tpg->compose.height || x + 8 >= tpg->compose.width)
		return;

	if (len > (tpg->compose.width - x) / 8)
		len = (tpg->compose.width - x) / 8;
	if (tpg->vflip)
		y = tpg->compose.height - y - 16;
	if (tpg->hflip)
		x = tpg->compose.width - x - 8;
	y += tpg->compose.top;
	x += tpg->compose.left;
	if (tpg->field == V4L2_FIELD_BOTTOM)
		first = 1;
	else if (tpg->field == V4L2_FIELD_SEQ_TB || tpg->field == V4L2_FIELD_SEQ_BT)
		div = 2;

	for (p = 0; p < tpg->planes; p++) {
		/* Print stream time */
#define PRINTSTR(PIXTYPE) do {	\
	PIXTYPE fg;	\
	PIXTYPE bg;	\
	memcpy(&fg, tpg->textfg[p], sizeof(PIXTYPE));	\
	memcpy(&bg, tpg->textbg[p], sizeof(PIXTYPE));	\
	\
	for (line = first; line < 16; line += step) {	\
		int l = tpg->vflip ? 15 - line : line; \
		PIXTYPE *pos = (PIXTYPE *)(basep[p][line & 1] + \
			       ((y * step + l) / div) * tpg->bytesperline[p] + \
			       x * sizeof(PIXTYPE));	\
		unsigned s;	\
	\
		for (s = 0; s < len; s++) {	\
			u8 chr = font8x16[text[s] * 16 + line];	\
	\
			if (tpg->hflip) { \
				pos[7] = (chr & (0x01 << 7) ? fg : bg);	\
				pos[6] = (chr & (0x01 << 6) ? fg : bg);	\
				pos[5] = (chr & (0x01 << 5) ? fg : bg);	\
				pos[4] = (chr & (0x01 << 4) ? fg : bg);	\
				pos[3] = (chr & (0x01 << 3) ? fg : bg);	\
				pos[2] = (chr & (0x01 << 2) ? fg : bg);	\
				pos[1] = (chr & (0x01 << 1) ? fg : bg);	\
				pos[0] = (chr & (0x01 << 0) ? fg : bg);	\
			} else { \
				pos[0] = (chr & (0x01 << 7) ? fg : bg);	\
				pos[1] = (chr & (0x01 << 6) ? fg : bg);	\
				pos[2] = (chr & (0x01 << 5) ? fg : bg);	\
				pos[3] = (chr & (0x01 << 4) ? fg : bg);	\
				pos[4] = (chr & (0x01 << 3) ? fg : bg);	\
				pos[5] = (chr & (0x01 << 2) ? fg : bg);	\
				pos[6] = (chr & (0x01 << 1) ? fg : bg);	\
				pos[7] = (chr & (0x01 << 0) ? fg : bg);	\
			} \
	\
			pos += tpg->hflip ? -8 : 8;	\
		}	\
	}	\
} while (0)

		switch (tpg->twopixelsize[p]) {
		case 2:
			PRINTSTR(u8); break;
		case 4:
			PRINTSTR(u16); break;
		case 6:
			PRINTSTR(x24); break;
		case 8:
			PRINTSTR(u32); break;
		}
	}
}
