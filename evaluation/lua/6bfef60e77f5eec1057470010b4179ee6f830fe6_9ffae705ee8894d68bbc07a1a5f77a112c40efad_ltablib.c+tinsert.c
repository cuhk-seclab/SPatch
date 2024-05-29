static int tinsert (lua_State *L) {
  int v = lua_gettop(L);  /* number of arguments */
  int e = aux_getn(L, 1) + LUA_FIRSTINDEX;  /* first empty element */
  int pos;  /* where to insert new element */
  if (v == 2)  /* called with only 2 arguments */
    pos = e;  /* insert new element at the end */
  else {
    pos = luaL_checkint(L, 2);  /* 2nd argument is the position */
    if (pos > e) e = pos;  /* `grow' array if necessary */
    v = 3;  /* function may be called with more than 3 args */
  }
  luaL_setn(L, 1, e - LUA_FIRSTINDEX + 1);  /* new size */
  while (--e >= pos) {  /* move up elements */
    lua_rawgeti(L, 1, e);
    lua_rawseti(L, 1, e+1);  /* t[e+1] = t[e] */
  }
  lua_pushvalue(L, v);
  lua_rawseti(L, 1, pos);  /* t[pos] = v */
  return 0;
}
