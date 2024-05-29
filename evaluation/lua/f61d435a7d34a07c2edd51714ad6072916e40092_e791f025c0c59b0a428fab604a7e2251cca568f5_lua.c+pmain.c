static int pmain (lua_State *L) {
  struct Smain *s = (struct Smain *)lua_touserdata(L, 1);
  int status;
  int interactive = 1;
  if (s->argv[0] && s->argv[0][0]) progname = s->argv[0];
  globalL = L;
  luaopen_stdlibs(L);  /* open libraries */
  status = handle_luainit(L);
  if (status == 0) {
    status = handle_argv(L, s->argv, &interactive);
    if (status == 0 && interactive) dotty(L);
  }
  s->status = status;
  return 0;
}
