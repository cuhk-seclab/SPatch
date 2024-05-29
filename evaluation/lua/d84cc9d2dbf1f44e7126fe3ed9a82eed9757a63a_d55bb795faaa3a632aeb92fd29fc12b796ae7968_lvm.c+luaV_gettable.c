StkId luaV_gettable (lua_State *L, const TValue *t, TValue *key, StkId val,
                     const Instruction *pc) {
  int loop;
  for (loop = 0; loop < MAXTAGLOOP; loop++) {
    const TValue *tm;
    if (ttistable(t)) {  /* `t' is a table? */
      Table *h = hvalue(t);
      const TValue *res = luaH_get(h, key); /* do a primitive set */
      if (!ttisnil(res) ||  /* result is no nil? */
          (tm = fasttm(L, h->metatable, TM_INDEX)) == NULL) { /* or no TM? */
        setobj2s(L, val, res);
        return L->base;
      }
      /* else will try the tag method */
    }
    else if (ttisnil(tm = luaT_gettmbyobj(L, t, TM_INDEX))) {
      L->ci->savedpc = pc;
      luaG_typeerror(L, t, "index");
    }
    if (ttisfunction(tm)) {
      L->ci->savedpc = pc;
      prepTMcall(L, tm, t, key);
      callTMres(L, val);
      return L->base;
    }
    t = tm;  /* else repeat with `tm' */ 
  }
  L->ci->savedpc = pc;
  luaG_runerror(L, "loop in gettable");
  return NULL;  /* to avoid warnings */
}
