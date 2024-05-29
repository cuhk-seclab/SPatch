static int gctm (lua_State *L) {
  void *lib = *(void **)luaL_checkudata(L, 1, "_LOADLIB");
  freelib(lib);
  return 0;
}
