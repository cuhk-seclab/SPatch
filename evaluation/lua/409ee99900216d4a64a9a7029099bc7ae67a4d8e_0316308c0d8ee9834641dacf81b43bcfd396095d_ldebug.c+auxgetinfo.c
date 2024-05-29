static int auxgetinfo (lua_State *L, const char *what, lua_Debug *ar,
                    StkId f, CallInfo *ci) {
  int status = 1;
  for (; *what; what++) {
    switch (*what) {
      case 'S': {
        funcinfo(ar, f);
        break;
      }
      case 'l': {
        ar->currentline = (ci) ? currentline(ci) : -1;
        break;
      }
      case 'u': {
        ar->nups = clvalue(f)->c.nupvalues;
        break;
      }
      case 'n': {
        ar->namewhat = (ci) ? getfuncname(ci, &ar->name) : NULL;
        if (ar->namewhat == NULL) {
          ar->namewhat = "";  /* not found */
          ar->name = NULL;
        }
        break;
      }
      case 'f': {
        setobj2s(L, L->top, f);
        break;
      }
      default: status = 0;  /* invalid option */
    }
  }
  return status;
}
