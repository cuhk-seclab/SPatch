static int io_tmpname (lua_State *L) {
#if !LUA_USE_TMPNAME
  luaL_error(L, "`tmpname' not supported");
  return 0;
#else
  char buff[L_tmpnam];
  if (tmpnam(buff) != buff)
    return luaL_error(L, "unable to generate a unique filename in `tmpname'");
  lua_pushstring(L, buff);
  return 1;
#endif
}
