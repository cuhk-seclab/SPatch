static int loader_C (lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  const char *fname = luaL_gsub(L, name, ".", LUA_DIRSEP);
  const char *path;
  const char *funcname;
  luaL_getfield(L, LUA_REGISTRYINDEX, "_PACKAGE.cpath");
  path = lua_tostring(L, -1);
  if (path == NULL)
    luaL_error(L, "`package.cpath' must be a string");
  fname = luaL_searchpath(L, fname, path);
  if (fname == NULL) return 0;  /* library not found in this path */
  funcname = luaL_gsub(L, name, ".", LUA_OFSEP);
  funcname = lua_pushfstring(L, "%s%s", POF, funcname);
  if (ll_loadfunc(L, fname, funcname) != 1)
    luaL_error(L, "error loading package `%s' (%s)", name, lua_tostring(L, -2));
  return 1;  /* library loaded successfully */
}
