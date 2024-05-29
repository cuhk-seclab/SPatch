static int handle_argv (lua_State *L, char *argv[], int *interactive) {
  if (argv[1] == NULL) {  /* no arguments? */
    *interactive = 0;
    if (stdin_is_tty())
      dotty(L);
    else
      dofile(L, NULL);  /* executes stdin as a file */
  }
  else {  /* other arguments; loop over them */
    int i;
    for (i = 1; argv[i] != NULL; i++) {
      if (argv[i][0] != '-') break;  /* not an option? */
      switch (argv[i][1]) {  /* option */
        case '-': {  /* `--' */
          if (argv[i][2] != '\0') {
            print_usage();
            return 1;
          }
          i++;  /* skip this argument */
          goto endloop;  /* stop handling arguments */
        }
        case '\0': {
          clearinteractive(interactive);
          dofile(L, NULL);  /* executes stdin as a file */
          break;
        }
        case 'i': {
          *interactive = 2;  /* force interactive mode after arguments */
          break;
        }
        case 'v': {
          clearinteractive(interactive);
          print_version();
          break;
        }
        case 'w': {
          if (lua_getmetatable(L, LUA_GLOBALSINDEX)) {
            lua_pushcfunction(L, checkvar);
            lua_setfield(L, -2, "__index");
          }
          break;
        }
        case 'e': {
          const char *chunk = argv[i] + 2;
          clearinteractive(interactive);
          if (*chunk == '\0') chunk = argv[++i];
          if (chunk == NULL) {
            print_usage();
            return 1;
          }
          if (dostring(L, chunk, "=<command line>") != 0)
            return 1;
          break;
        }
        case 'l': {
          const char *filename = argv[i] + 2;
          if (*filename == '\0') filename = argv[++i];
          if (filename == NULL) {
            print_usage();
            return 1;
          }
          if (dolibrary(L, filename))
            return 1;  /* stop if file fails */
          break;
        }
        default: {
          clearinteractive(interactive);
          print_usage();
          return 1;
        }
      }
    } endloop:
    if (argv[i] != NULL) {
      const char *filename = argv[i];
      int narg = getargs(L, argv, i);  /* collect arguments */
      int status;
      lua_setglobal(L, "arg");
      clearinteractive(interactive);
      status = luaL_loadfile(L, filename);
      lua_insert(L, -(narg+1));
      if (status == 0)
        status = docall(L, narg, 0);
      else
        lua_pop(L, narg);      
      return report(L, status);
    }
  }
  return 0;
}
