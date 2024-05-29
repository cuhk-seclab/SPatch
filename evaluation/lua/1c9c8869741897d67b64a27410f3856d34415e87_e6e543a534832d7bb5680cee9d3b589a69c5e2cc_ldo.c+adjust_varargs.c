static StkId adjust_varargs (lua_State *L, int nfixargs, int actual,
                             int style) {
  int i;
  Table *htab = NULL;
  StkId base, fixed;
  if (actual < nfixargs) {
    for (; actual < nfixargs; ++actual)
      setnilvalue(L->top++);
  }
#if LUA_COMPAT_VARARG
  if (style != NEWSTYLEVARARG) {  /* compatibility with old-style vararg */
    int nvar = actual - nfixargs;  /* number of extra arguments */
    luaC_checkGC(L);
    htab = luaH_new(L, nvar, 1);  /* create `arg' table */
    for (i=0; i<nvar; i++)  /* put extra arguments into `arg' table */
      setobj2n(L, luaH_setnum(L, htab, i+1), L->top - nvar + i);
    /* store counter in field `n' */
    setnvalue(luaH_setstr(L, htab, luaS_newliteral(L, "n")),
                                   cast(lua_Number, nvar));
  }
#endif
  /* move fixed parameters to final position */
  fixed = L->top - actual;  /* first fixed argument */
  base = L->top;  /* final position of first argument */
  for (i=0; i<nfixargs; i++) {
    setobjs2s(L, L->top++, fixed+i);
    setnilvalue(fixed+i);
  }
  /* add `arg' parameter */
  if (htab) {
    sethvalue(L, L->top++, htab);
    lua_assert(iswhite(obj2gco(htab)));
  }
  return base;
}
