void *luaM_realloc_ (lua_State *L, void *block, size_t osize, size_t nsize) {
  global_State *g = G(L);
  lua_assert((osize == 0) == (block == NULL));
  block = (*g->realloc)(g->ud, block, osize, nsize);
  if (block == NULL && nsize > 0)
    luaD_throw(L, LUA_ERRMEM);
  lua_assert((nsize == 0) == (block == NULL));
  g->totalbytes = (g->totalbytes - osize) + nsize;
#if 0
  { /* auxiliar patch to monitor garbage collection */
    static unsigned long total = 0;  /* our "time" */
    static lu_mem last = 0;  /* last totalmem that generated an output */
    static FILE *f = NULL;  /* output file */
    if (nsize <= osize) total += 1;  /* "time" always grow */
    else total += (nsize - osize);
    if ((int)g->totalbytes - (int)last > 1000 ||
        (int)g->totalbytes - (int)last < -1000) {
      last = g->totalbytes;
      if (f == NULL) f = fopen("trace", "w");
      fprintf(f, "%lu %u %u %u %d\n", total, g->totalbytes, g->GCthreshold,
                                      g->estimate, g->gcstate);
    }
  }
#endif
  return block;
}
