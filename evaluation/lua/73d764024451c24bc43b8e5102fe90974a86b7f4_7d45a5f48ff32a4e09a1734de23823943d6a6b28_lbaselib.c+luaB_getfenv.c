static int luaB_getfenv (lua_State *L) {
  getfunc(L);
  if (lua_iscfunction(L, -1))  /* is a C function? */
    lua_pushvalue(L, LUA_GLOBALSINDEX);  /* return the global env. */
  else
    lua_getfenv(L, -1);
  return 1;
}
