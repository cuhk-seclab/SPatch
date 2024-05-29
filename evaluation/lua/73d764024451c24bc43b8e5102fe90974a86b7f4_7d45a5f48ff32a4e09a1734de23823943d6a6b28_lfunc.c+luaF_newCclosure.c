Closure *luaF_newCclosure (lua_State *L, int nelems) {
  Closure *c = cast(Closure *, luaM_malloc(L, sizeCclosure(nelems)));
  luaC_link(L, obj2gco(c), LUA_TFUNCTION);
  c->c.isC = 1;
  c->c.nupvalues = cast(lu_byte, nelems);
  return c;
}
