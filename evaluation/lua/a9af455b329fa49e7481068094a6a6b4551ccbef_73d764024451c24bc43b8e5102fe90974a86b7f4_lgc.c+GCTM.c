static void GCTM (lua_State *L) {
  global_State *g = G(L);
  GCObject *o = g->tmudata->gch.next;  /* get first element */
  Udata *udata = rawgco2u(o);
  const TValue *tm;
  /* remove udata from `tmudata' */
  if (o == g->tmudata)  /* last element? */
    g->tmudata = NULL;
  else
    g->tmudata->gch.next = udata->uv.next;
  udata->uv.next = g->mainthread->next;  /* return it to `root' list */
  g->mainthread->next = o;
  makewhite(g, o);
  tm = fasttm(L, udata->uv.metatable, TM_GC);
  if (tm != NULL) {
    lu_byte oldah = L->allowhook;
    lu_mem oldt = g->GCthreshold;
    L->allowhook = 0;  /* stop debug hooks during GC tag method */
    g->GCthreshold = 2*g->totalbytes;  /* avoid GC steps */
    setobj2s(L, L->top, tm);
    setuvalue(L, L->top+1, udata);
    L->top += 2;
    luaD_call(L, L->top - 2, 0);
    L->allowhook = oldah;  /* restore hooks */
    g->GCthreshold = oldt;  /* restore threshold */
  }
}
