void luaC_fullgc (lua_State *L) {
  global_State *g = G(L);
  if (g->gcstate <= GCSpropagate) {
    /* reset sweep marks to sweep all elements (returning them to white) */
    g->sweepstrgc = 0;
    g->sweepgc = &g->rootgc;
    /* reset other collector lists */
    g->gray = NULL;
    g->grayagain = NULL;
    g->weak = NULL;
    g->gcstate = GCSsweepstring;
  }
  lua_assert(g->gcstate != GCSpause && g->gcstate != GCSpropagate);
  /* finish any pending sweep phase */
  while (g->gcstate != GCSfinalize) {
    lua_assert(g->gcstate == GCSsweepstring || g->gcstate == GCSsweep);
    singlestep(L);
  }
  markroot(L);
  while (g->gcstate != GCSpause) {
    singlestep(L);
  }
  setthreshold(g);
}
