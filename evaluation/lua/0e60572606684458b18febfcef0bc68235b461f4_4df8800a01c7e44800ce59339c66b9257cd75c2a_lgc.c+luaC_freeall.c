void luaC_freeall (lua_State *L) {
  global_State *g = G(L);
  int i;
  freelist(L, &g->rootgc);
  for (i = 0; i < g->strt.size; i++)  /* free all string lists */
    freelist(L, &G(L)->strt.hash[i]);
}
