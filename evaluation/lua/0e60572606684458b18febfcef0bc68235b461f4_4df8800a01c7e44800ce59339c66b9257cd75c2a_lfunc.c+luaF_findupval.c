UpVal *luaF_findupval (lua_State *L, StkId level) {
  global_State *g = G(L);
  GCObject **pp = &L->openupval;
  UpVal *p;
  UpVal *uv;
  while ((p = ngcotouv(*pp)) != NULL && p->v >= level) {
    lua_assert(p->v != &p->u.value);
    if (p->v == level) {  /* found a corresponding upvalue? */
      if (isdead(G(L), obj2gco(p)))  /* is it dead? */
        changewhite(obj2gco(p));  /* ressurect it */
      return p;
    }
    pp = &p->next;
  }
  uv = luaM_new(L, UpVal);  /* not found: create a new one */
  uv->tt = LUA_TUPVAL;
  uv->marked = luaC_white(g);
  uv->v = level;  /* current value lives in the stack */
  uv->next = *pp;  /* chain it in the proper position */
  *pp = obj2gco(uv);
  uv->u.l.prev = &g->uvhead;  /* double link it in `uvhead' list */
  uv->u.l.next = g->uvhead.u.l.next;
  uv->u.l.next->u.l.prev = uv;
  g->uvhead.u.l.next = uv;
  lua_assert(uv->u.l.next->u.l.prev == uv && uv->u.l.prev->u.l.next == uv);
  return uv;
}
