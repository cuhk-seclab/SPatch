LUALIB_API int luaopen_stdlibs (lua_State *L) {
  const luaL_reg *lib = lualibs;
  for (; lib->func; lib++) {
    lib->func(L);  /* open library */
    lua_settop(L, 0);  /* discard any results */
  }
 return 0;
}
