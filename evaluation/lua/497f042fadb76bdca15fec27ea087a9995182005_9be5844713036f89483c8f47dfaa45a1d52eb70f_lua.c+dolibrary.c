static int dolibrary (lua_State *L, const char *name) {
  luaL_getfield(L, LUA_GLOBALSINDEX, "package.path");
  if (!lua_isstring(L, -1)) {
    l_message(progname, "`package.path' must be a string");
    return 1;
  }
  name = luaL_searchpath(L, name, lua_tostring(L, -1));
  if (name == NULL) return report(L, 1);
  else  return dofile(L, name);
}
