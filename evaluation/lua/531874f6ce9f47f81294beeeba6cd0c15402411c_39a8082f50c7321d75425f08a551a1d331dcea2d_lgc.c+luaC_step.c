void luaC_step (lua_State *L) {
  global_State *g = G(L);
  l_mem lim = (g->totalbytes - (g->GCthreshold - GCSTEPSIZE)) * STEPMUL;
/*printf("step(%c): ", g->gcgenerational?'g':' ');*/
  do {
    lim -= singlestep(L);
    if (g->gcstate == GCSpause)
      break;
  } while (lim > 0);
  if (g->gcstate != GCSpause)
    g->GCthreshold = g->totalbytes + GCSTEPSIZE;  /* - lim/STEPMUL; */
  else {
    lua_assert(g->totalbytes >= g->estimate);
    lua_assert(g->gcstate == GCSpause);
    g->GCthreshold = 2*g->estimate;
  }
}
