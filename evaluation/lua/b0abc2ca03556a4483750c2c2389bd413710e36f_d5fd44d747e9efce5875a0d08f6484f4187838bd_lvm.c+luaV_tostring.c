int luaV_tostring (lua_State *L, StkId obj) {
  if (!ttisnumber(obj))
    return 0;
  else {
    char s[LUAI_MAXNUMBER2STR];
    lua_number2str(s, nvalue(obj));
    setsvalue2s(L, obj, luaS_new(L, s));
    return 1;
  }
}
