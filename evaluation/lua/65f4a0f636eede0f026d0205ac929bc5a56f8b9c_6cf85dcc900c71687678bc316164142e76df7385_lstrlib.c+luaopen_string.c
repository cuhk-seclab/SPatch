LUALIB_API int luaopen_string (lua_State *L) {
  luaL_openlib(L, LUA_STRLIBNAME, strlib, 0);
  createmetatable(L);
  return 1;
}
