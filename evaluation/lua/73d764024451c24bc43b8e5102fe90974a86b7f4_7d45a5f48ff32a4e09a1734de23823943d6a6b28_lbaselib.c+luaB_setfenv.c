static int luaB_setfenv (lua_State *L) {
  luaL_checktype(L, 2, LUA_TTABLE);
  getfunc(L);
  lua_pushvalue(L, 2);
  if (lua_isnumber(L, 1) && lua_tonumber(L, 1) == 0) {
    lua_replace(L, LUA_GLOBALSINDEX);
    return 0;
  }
  else if (lua_iscfunction(L, -2) || lua_setfenv(L, -2) == 0)
    luaL_error(L, "`setfenv' cannot change environment of given object");
  return 1;
}
