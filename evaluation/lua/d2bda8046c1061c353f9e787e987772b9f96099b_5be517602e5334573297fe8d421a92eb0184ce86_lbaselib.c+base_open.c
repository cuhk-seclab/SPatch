static void base_open (lua_State *L) {
  lua_pushvalue(L, LUA_GLOBALSINDEX);
  luaL_openlib(L, NULL, base_funcs, 0);  /* open lib into global table */
  lua_pushliteral(L, LUA_VERSION);
  lua_setglobal(L, "_VERSION");  /* set global _VERSION */
  /* `ipairs' and `pairs' need auxiliary functions as upvalues */
  auxopen(L, "ipairs", luaB_ipairs, ipairsaux);
  auxopen(L, "pairs", luaB_pairs, luaB_next);
  /* `newproxy' needs a weaktable as upvalue */
  lua_newtable(L);  /* new table `w' */
  lua_pushvalue(L, -1);  /* `w' will be its own metatable */
  lua_setmetatable(L, -2);
  lua_pushliteral(L, "kv");
  lua_setfield(L, -2, "__mode");  /* metatable(w).__mode = "kv" */
  lua_pushcclosure(L, luaB_newproxy, 1);
  lua_setglobal(L, "newproxy");  /* set global `newproxy' */
  /* create register._LOADED to track loaded modules */
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, "_LOADED");
  /* set global _G */
  lua_pushvalue(L, LUA_GLOBALSINDEX);
  lua_setglobal(L, "_G");
}
