void luaF_close (lua_State *L, StkId level) {
  UpVal *uv;
  global_State *g = G(L);
  while ((uv = ngcotouv(L->openupval)) != NULL && uv->v >= level) {
    GCObject *o = obj2gco(uv);
    lua_assert(!isblack(o) && uv->v != &uv->u.value);
    L->openupval = uv->next;  /* remove from `open' list */
    if (isdead(g, o))
      luaF_freeupval(L, uv);  /* free upvalue */
    else {
      unlinkupval(uv);
      setobj(L, &uv->u.value, uv->v);
        gray2black(o);  /* closed upvalues are never gray */
        luaC_barrier(L, uv, uv->v);
      uv->v = &uv->u.value;  /* now current value lives here */
      luaC_linkupval(L, uv);  /* link upvalue into `gcroot' list */
    }
  }
}
