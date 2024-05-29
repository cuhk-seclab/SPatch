static const char *loadC (lua_State *L, const char *fname, const char *name) {
  const char *path;
  const char *funcname;
  luaL_getfield(L, LUA_GLOBALSINDEX, "package.cpath");
  path = lua_tostring(L, -1);
  if (path == NULL)
    luaL_error(L, "global _CPATH must be a string");
  fname = luaL_searchpath(L, fname, path);
  if (fname == NULL) return path;  /* library not found in this path */
  funcname = luaL_gsub(L, name, ".", "");
  funcname = lua_pushfstring(L, "%s%s", LUA_POF, funcname);
  if (loadlib(L, fname, funcname) != 0)
    luaL_error(L, "error loading package `%s' (%s)", name, lua_tostring(L, -1));
  return NULL;  /* library loaded successfully */
}
