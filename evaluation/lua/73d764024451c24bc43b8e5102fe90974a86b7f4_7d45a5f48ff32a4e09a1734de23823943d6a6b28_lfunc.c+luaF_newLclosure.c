Closure *luaF_newLclosure (lua_State *L, int nelems, TValue *e) {
  Closure *c = cast(Closure *, luaM_malloc(L, sizeLclosure(nelems)));
  luaC_link(L, obj2gco(c), LUA_TFUNCTION);
  c->l.isC = 0;
  c->l.g = *e;
  c->l.nupvalues = cast(lu_byte, nelems);
  while (nelems--) c->l.upvals[nelems] = NULL;
  return c;
}
