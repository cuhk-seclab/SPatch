static GCObject **sweeplist (lua_State *L, GCObject **p, lu_mem count) {
  GCObject *curr;
  global_State *g = G(L);
  int whitebit = otherwhite(g);
  int deadmask = whitebit | FIXEDMASK;
  while ((curr = *p) != NULL && count-- > 0) {
      sweepwholelist(L, &gco2th(curr)->openupval);
    if ((curr->gch.marked ^ whitebit) & deadmask) {
      lua_assert(!isdead(g, curr) || testbit(curr->gch.marked, FIXEDBIT));
      makewhite(g, curr);
        sweepwholelist(L, &gco2th(curr)->openupval);
      p = &curr->gch.next;
    }
    else {
      lua_assert(isdead(g, curr));
      *p = curr->gch.next;
      if (curr == g->rootgc)  /* is the first element of the list? */
        g->rootgc = curr->gch.next;  /* adjust first */
      freeobj(L, curr);
    }
  }
  return p;
}
