static int loader_Lua (lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  const char *fname = luaL_gsub(L, name, ".", LUA_DIRSEP);
  const char *path = NULL;
#if LUA_COMPAT_PATH
  /* try first `LUA_PATH' for compatibility */
  lua_pushstring(L, "LUA_PATH");
  lua_rawget(L, LUA_GLOBALSINDEX);
  path = lua_tostring(L, -1);
#endif
  if (!path) {
    lua_pop(L, 1);
    lua_getfield(L, LUA_ENVIRONINDEX, "path");
    path = lua_tostring(L, -1);
  }
  if (path == NULL)
    luaL_error(L, "`package.path' must be a string");
  fname = luaL_searchpath(L, fname, path);
  if (fname == NULL) return 0;  /* library not found in this path */
  if (luaL_loadfile(L, fname) != 0)
    luaL_error(L, "error loading package `%s' (%s)", name, lua_tostring(L, -1));
  return 1;  /* library loaded successfully */
}
