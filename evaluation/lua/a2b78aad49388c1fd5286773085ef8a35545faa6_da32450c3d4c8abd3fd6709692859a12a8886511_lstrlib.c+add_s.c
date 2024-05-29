static void add_s (MatchState *ms, luaL_Buffer *b,
                   const char *s, const char *e) {
  lua_State *L = ms->L;
  if (lua_isstring(L, 3)) {
    size_t l;
    const char *news = lua_tolstring(L, 3, &l);
    size_t i;
    for (i=0; i<l; i++) {
      if (news[i] != L_ESC)
        luaL_putchar(b, news[i]);
      else {
        i++;  /* skip ESC */
        if (!isdigit(uchar(news[i])))
          luaL_putchar(b, news[i]);
        else {
          int level = check_capture(ms, news[i]);
          push_onecapture(ms, level);
          luaL_addvalue(b);  /* add capture to accumulated result */
        }
      }
    }
  }
  else {  /* is a function */
    int n;
    lua_pushvalue(L, 3);
    n = push_captures(ms, s, e);
    lua_call(L, n, 1);
    if (lua_isstring(L, -1))
      luaL_addvalue(b);  /* add return to accumulated result */
    else
      lua_pop(L, 1);  /* function result is not a string: pop it */
  }
}
