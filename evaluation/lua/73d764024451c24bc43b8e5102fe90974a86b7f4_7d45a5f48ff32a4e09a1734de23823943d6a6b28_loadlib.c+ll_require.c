static int ll_require (lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  int i;
  lua_settop(L, 1);
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
  lua_getfield(L, 2, name);
  if (lua_toboolean(L, -1))  /* is it there? */
    return 1;  /* package is already loaded; return its result */
  /* else must load it; first mark it as loaded */
  lua_pushboolean(L, 1);
  lua_setfield(L, 2, name);  /* _LOADED[name] = true */
  /* iterate over available loaders */
  luaL_getfield(L, LUA_REGISTRYINDEX, "_PACKAGE.loaders");
  if (!lua_istable(L, -1))
    luaL_error(L, "`package.loaders' must be a table");
  for (i=1;; i++) {
    lua_rawgeti(L, -1, i);  /* get a loader */
    if (lua_isnil(L, -1))
      return luaL_error(L, "package `%s' not found", name);
    lua_pushstring(L, name);
    lua_call(L, 1, 1);  /* call it */
    if (lua_isnil(L, -1)) lua_pop(L, 1);
    else break;  /* module loaded successfully */
  }
  lua_pushvalue(L, 1);  /* pass name as argument to module */
  lua_call(L, 1, 1);  /* run loaded module */
  if (!lua_isnil(L, -1))  /* non-nil return? */
    lua_setfield(L, 2, name);  /* update _LOADED[name] with returned value */
  lua_getfield(L, 2, name);  /* return _LOADED[name] */
  return 1;
}
