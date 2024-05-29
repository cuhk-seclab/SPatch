static int getargs (lua_State *L, char *argv[], int n) {
  int i, narg;
  for (i=n+1; argv[i]; i++) {
    luaL_checkstack(L, 1, "too many arguments to script");
    lua_pushstring(L, argv[i]);
  }
  narg = i-(n+1);  /* number of arguments to the script (not to `lua.c') */
  lua_newtable(L);
  for (i=0; argv[i]; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i - n);
  }
  return narg;
}
