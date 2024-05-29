size_t luaC_separateudata (lua_State *L, int all) {
  size_t deadmem = 0;
  GCObject **p = &G(L)->firstudata;
  GCObject *curr;
  GCObject *collected = NULL;  /* to collect udata with gc event */
  GCObject **lastcollected = &collected;
  while ((curr = *p)->gch.tt == LUA_TUSERDATA) {
    if (!(iswhite(curr) || all) || isfinalized(gco2u(curr)))
      p = &curr->gch.next;  /* don't bother with them */
    else if (fasttm(L, gco2u(curr)->metatable, TM_GC) == NULL) {
      markfinalized(gco2u(curr));  /* don't need finalization */
      p = &curr->gch.next;
    }
    else {  /* must call its gc method */
      deadmem += sizeudata(gco2u(curr)->len);
      markfinalized(gco2u(curr));
      *p = curr->gch.next;
      curr->gch.next = NULL;  /* link `curr' at the end of `collected' list */
      *lastcollected = curr;
      lastcollected = &curr->gch.next;
    }
  }
  lua_assert(curr == obj2gco(G(L)->mainthread));
  /* insert collected udata with gc event into `tmudata' list */
  *lastcollected = G(L)->tmudata;
  G(L)->tmudata = collected;
  return deadmem;
}
