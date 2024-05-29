static int currentpc (lua_State *L, CallInfo *ci) {
  UNUSED(L);
  if (!isLua(ci)) return -1;  /* function is not a Lua function? */
  return pcRel(ci->savedpc, ci_func(ci)->l.p);
}
