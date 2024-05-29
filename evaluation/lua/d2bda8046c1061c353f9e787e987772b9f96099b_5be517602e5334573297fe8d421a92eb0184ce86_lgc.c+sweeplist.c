static GCObject **sweeplist (lua_State *L, GCObject **p, lu_int32 count) {
  GCObject *curr;
  global_State *g = G(L);
  int whitebit = otherwhite(g);
  int deadmask = whitebit | FIXEDMASK;
  int generational = g->gcgenerational;
  while ((curr = *p) != NULL && count-- > 0) {
    if ((curr->gch.marked ^ whitebit) & deadmask) {
      lua_assert(!isdead(g, curr) || testbit(curr->gch.marked, FIXEDBIT));
      if (!generational || isdead(g, curr))
        makewhite(g, curr);
      if (curr->gch.tt == LUA_TTHREAD)
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
