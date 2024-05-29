static int str_format (lua_State *L) {
  int arg = 1;
  size_t sfl;
  const char *strfrmt = luaL_checklstring(L, arg, &sfl);
  const char *strfrmt_end = strfrmt+sfl;
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  while (strfrmt < strfrmt_end) {
    if (*strfrmt != ESC)
      luaL_putchar(&b, *strfrmt++);
    else if (*++strfrmt == ESC)
      luaL_putchar(&b, *strfrmt++);  /* %% */
    else { /* format item */
      char form[MAX_FORMAT];  /* to store the format (`%...') */
      char buff[MAX_ITEM];  /* to store the formatted item */
      int hasprecision = 0;
      if (isdigit(uchar(*strfrmt)) && *(strfrmt+1) == '$')
        return luaL_error(L, "obsolete option (d$) to `format'");
      arg++;
      strfrmt = scanformat(L, strfrmt, form, &hasprecision);
      switch (*strfrmt++) {
        case 'c':  case 'd':  case 'i': {
          sprintf(buff, form, luaL_checkint(L, arg));
          break;
        }
        case 'o':  case 'u':  case 'x':  case 'X': {
          sprintf(buff, form, (unsigned int)(luaL_checknumber(L, arg)));
          break;
        }
        case 'e':  case 'E': case 'f':
        case 'g': case 'G': {
          sprintf(buff, form, luaL_checknumber(L, arg));
          break;
        }
        case 'q': {
          luaI_addquoted(L, &b, arg);
          continue;  /* skip the `addsize' at the end */
        }
        case 's': {
          size_t l;
          const char *s = luaL_checklstring(L, arg, &l);
          if (!hasprecision && l >= 100) {
            /* no precision and string is too long to be formatted;
               keep original string */
            lua_pushvalue(L, arg);
            luaL_addvalue(&b);
            continue;  /* skip the `addsize' at the end */
          }
          else {
            sprintf(buff, form, s);
            break;
          }
        }
        default: {  /* also treat cases `pnLlh' */
          return luaL_error(L, "invalid option to `format'");
        }
      }
      luaL_addlstring(&b, buff, strlen(buff));
    }
  }
  luaL_pushresult(&b);
  return 1;
}
