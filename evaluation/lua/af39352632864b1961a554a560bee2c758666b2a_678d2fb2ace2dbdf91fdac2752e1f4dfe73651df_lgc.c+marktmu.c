static void marktmu (global_State *g) {
  GCObject *u;
  for (u = g->tmudata; u; u = u->gch.next) {
    makewhite(g, u);  /* may be marked, if left from previous GC */
    reallymarkobject(g, u);
  }
}
