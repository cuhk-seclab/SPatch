static int tinsert (lua_State *L) {
  int e = aux_getn(L, 1) + 1;  /* first empty element */
  int pos;  /* where to insert new element */
  if (lua_isnone(L, 3))  /* called with only 2 arguments */
    pos = e;  /* insert new element at the end */
  else {
    int i;
    pos = luaL_checkint(L, 2);  /* 2nd argument is the position */
    if (pos > e) e = pos;  /* `grow' array if necessary */
    lua_settop(L, 3);  /* function may be called with more than 3 args */
    for (i = e; i > pos; i--) {  /* move up elements */
      lua_rawgeti(L, 1, i-1);
      lua_rawseti(L, 1, i);  /* t[i] = t[i-1] */
    }
  }
  luaL_setn(L, 1, e);  /* new size */
  lua_rawseti(L, 1, pos);  /* t[pos] = v */
  return 0;
}
