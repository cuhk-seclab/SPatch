LUALIB_API int luaopen_loadlib (lua_State *L) {
  const char *path;
  /* create new type _LOADLIB */
  luaL_newmetatable(L, "_LOADLIB");
  lua_pushcfunction(L, gctm);
  lua_setfield(L, -2, "__gc");
  /* create `package' table */
  lua_newtable(L);
  lua_pushvalue(L, -1);
  lua_setglobal(L, "package");
  /* set field `path' */
  path = getenv(LUA_PATH);
  if (path == NULL) path = LUA_PATH_DEFAULT;
  lua_pushstring(L, path);
  lua_setfield(L, -2, "path");
  /* set field `cpath' */
  path = getenv(LUA_CPATH);
  if (path == NULL) path = LUA_CPATH_DEFAULT;
  lua_pushstring(L, path);
  lua_setfield(L, -2, "cpath");
  /* set field `loaded' */
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
  lua_setfield(L, -2, "loaded");
  /* set field `preload' */
  lua_getfield(L, LUA_REGISTRYINDEX, "_PRELOAD");
  lua_setfield(L, -2, "preload");
  lua_pushvalue(L, LUA_GLOBALSINDEX);
  luaL_openlib(L, NULL, ll_funcs, 0);  /* open lib into global table */
  return 1;
}
