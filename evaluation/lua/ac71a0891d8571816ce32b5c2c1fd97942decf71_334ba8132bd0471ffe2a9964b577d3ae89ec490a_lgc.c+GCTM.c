static void GCTM (lua_State *L) {
  global_State *g = G(L);
  GCObject *o = g->tmudata;
  Udata *udata = rawgco2u(o);
  const TValue *tm;
  g->tmudata = udata->uv.next;  /* remove udata from `tmudata' */
  udata->uv.next = g->mainthread->next;  /* return it to `root' list */
  g->mainthread->next = o;
  makewhite(g, o);
  tm = fasttm(L, udata->uv.metatable, TM_GC);
  if (tm != NULL) {
    lu_byte oldah = L->allowhook;
    L->allowhook = 0;  /* stop debug hooks during GC tag method */
    setobj2s(L, L->top, tm);
    setuvalue(L, L->top+1, udata);
    L->top += 2;
    luaD_call(L, L->top - 2, 0);
    L->allowhook = oldah;  /* restore hooks */
  }
}
