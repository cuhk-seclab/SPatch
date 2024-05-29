LUALIB_API int luaopen_stdlibs (lua_State *L) {
  const luaL_reg *lib = lualibs;
  for (; lib->func; lib++) {
    lib->func(L);  /* open library */
    lua_settop(L, 0);  /* discard any results */
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_replace(L, LUA_ENVIRONINDEX);  /* restore environment */
  }
  lua_pop(L, 1);
  return 0;
}
