LUALIB_API int luaopen_math (lua_State *L) {
  luaL_openlib(L, LUA_MATHLIBNAME, mathlib, 0);
  lua_pushnumber(L, PI);
  lua_setfield(L, -2, "pi");
  return 1;
}
