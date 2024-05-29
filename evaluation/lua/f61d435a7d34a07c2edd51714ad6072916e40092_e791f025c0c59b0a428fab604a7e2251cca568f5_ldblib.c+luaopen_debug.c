LUALIB_API int luaopen_debug (lua_State *L) {
  luaL_openlib(L, LUA_DBLIBNAME, dblib, 0);
  return 1;
}
