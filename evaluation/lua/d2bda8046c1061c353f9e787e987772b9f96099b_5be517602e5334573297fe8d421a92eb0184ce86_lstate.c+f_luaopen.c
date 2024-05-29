static void f_luaopen (lua_State *L, void *ud) {
  Udata *u;  /* head of udata list */
  global_State *g = G(L);
  UNUSED(ud);
  u = luaM_new(L, Udata);
  u->uv.len = 0;
  u->uv.metatable = NULL;
  g->firstudata = obj2gco(u);
  luaC_link(L, obj2gco(u), LUA_TUSERDATA);
  setbit(u->uv.marked, FIXEDBIT);
  setbit(L->marked, FIXEDBIT);
  stack_init(L, L);  /* init stack */
  sethvalue(L, gt(L), luaH_new(L, 0, 20));  /* table of globals */
  hvalue(gt(L))->metatable = luaH_new(L, 0, 0);  /* globals metatable */
  sethvalue(L, registry(L), luaH_new(L, 6, 20));  /* registry */
  luaS_resize(L, MINSTRTABSIZE);  /* initial size of string table */
  luaT_init(L);
  luaX_init(L);
  luaS_fix(luaS_newliteral(L, MEMERRMSG));
  g->GCthreshold = 4*g->totalbytes;
}
