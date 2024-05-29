static int luaB_collectgarbage (lua_State *L) {
  static const char *const opts[] = {"stop", "restart", "collect", "count",
                                     "step", NULL};
  static const int optsnum[] = {LUA_GCSTOP, LUA_GCRESTART,
                                LUA_GCCOLLECT, LUA_GCCOUNT, LUA_GCSTEP};
  int o = luaL_findstring(luaL_optstring(L, 1, "collect"), opts);
  int ex = luaL_optint(L, 2, 0);
  luaL_argcheck(L, o >= 0, 1, "invalid option");
  lua_pushinteger(L, lua_gc(L, optsnum[o], ex));
  return 1;
}
