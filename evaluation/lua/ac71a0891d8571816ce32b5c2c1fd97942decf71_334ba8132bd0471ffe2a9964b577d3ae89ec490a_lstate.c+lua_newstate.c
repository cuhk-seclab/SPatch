LUA_API lua_State *lua_newstate (lua_Alloc f, void *ud) {
  lua_State *L;
  global_State *g;
  void *l = (*f)(ud, NULL, 0, state_size(LG));
  if (l == NULL) return NULL;
  L = tostate(l);
  g = &((LG *)L)->g;
  L->next = NULL;
  g->currentwhite = bitmask(WHITE0BIT);
  preinit_state(L, g);
  g->realloc = f;
  g->ud = ud;
  g->mainthread = L;
  g->GCthreshold = 0;  /* mark it as unfinished state */
  g->strt.size = 0;
  g->strt.nuse = 0;
  g->strt.hash = NULL;
  setnilvalue(registry(L));
  luaZ_initbuffer(L, &g->buff);
  g->panic = NULL;
  g->gcstate = GCSpause;
  g->rootgc = obj2gco(L);
  g->sweepstrgc = 0;
  g->sweepgc = &g->rootgc;
  g->firstudata = NULL;
  g->gray = NULL;
  g->grayagain = NULL;
  g->weak = NULL;
  g->tmudata = NULL;
  g->totalbytes = sizeof(LG);
  g->gcpace = 200;  /* 200% (wait memory to double before next collection) */
  g->gcstepmul = 200;  /* GC runs `twice the speed' of memory allocation */
  g->gcdept = 0;
  if (luaD_rawrunprotected(L, f_luaopen, NULL) != 0) {
    /* memory allocation error: free partial state */
    close_state(L);
    L = NULL;
  }
  else
    lua_userstateopen(L);
  return L;
}
