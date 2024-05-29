void luaF_close (lua_State *L, StkId level) {
  UpVal *uv;
  global_State *g = G(L);
  while ((uv = ngcotouv(L->openupval)) != NULL && uv->v >= level) {
    GCObject *o = obj2gco(uv);
    lua_assert(!isblack(o));
    L->openupval = uv->next;  /* remove from `open' list */
    if (isdead(g, o))
      luaM_free(L, uv);  /* free upvalue */
    else {
      setobj(L, &uv->value, uv->v);
      uv->v = &uv->value;  /* now current value lives here */
      luaC_linkupval(L, uv);  /* link upvalue into `gcroot' list */
    }
  }
}
