UpVal *luaF_findupval (lua_State *L, StkId level) {
  GCObject **pp = &L->openupval;
  UpVal *p;
  UpVal *uv;
  while ((p = ngcotouv(*pp)) != NULL && p->v >= level) {
    if (p->v == level) return p;
    pp = &p->next;
  }
  uv = luaM_new(L, UpVal);  /* not found: create a new one */
  uv->tt = LUA_TUPVAL;
  uv->marked = luaC_white(G(L));
  uv->v = level;  /* current value lives in the stack */
  uv->next = *pp;  /* chain it in the proper position */
  *pp = obj2gco(uv);
  return uv;
}
