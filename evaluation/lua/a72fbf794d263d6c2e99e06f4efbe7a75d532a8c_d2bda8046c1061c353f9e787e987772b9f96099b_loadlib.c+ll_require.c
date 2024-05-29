static int ll_require (lua_State *L) {
  const char *name = luaL_checkstring(L, 1);
  lua_settop(L, 1);
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
  lua_getfield(L, 2, name);
  if (lua_toboolean(L, -1))  /* is it there? */
    return 1;  /* package is already loaded; return its result */
  /* else must load it; first mark it as loaded */
  lua_pushboolean(L, 1);
  lua_setfield(L, 2, name);  /* _LOADED[name] = true */
  /* check whether it is preloaded */
  lua_getfield(L, LUA_REGISTRYINDEX, "_PRELOAD");
  lua_getfield(L, -1, name);
  if (lua_isnil(L, -1)) {  /* no preload function for that module? */
    const char *fname = luaL_gsub(L, name, ".", LUA_DIRSEP);
    const char *cpath = loadC(L, fname, name);  /* try a C module */
    if (cpath) {  /* not found? */
      const char *path = loadLua(L, fname, name);  /* try a Lua module */
      if (path) {  /* yet not found? */
        lua_pushnil(L);
        lua_setfield(L, 2, name);  /* unmark _LOADED[name] */
        return luaL_error(L, "package `%s' not found\n"
                             "  cpath: %s\n  path: %s",
                             name, cpath, path);
      }
    }
  }
  lua_pushvalue(L, 1);  /* pass name as argument to module */
  lua_call(L, 1, 1);  /* run loaded module */
  if (!lua_isnil(L, -1))  /* non-nil return? */
    lua_setfield(L, 2, name);  /* update _LOADED[name] with returned value */
  lua_getfield(L, 2, name);  /* return _LOADED[name] */
  return 1;
}
