LUA_API int lua_setfenv (lua_State *L, int idx) {
  StkId o;
  int res = 0;
  lua_lock(L);
  api_checknelems(L, 1);
  o = index2adr(L, idx);
  api_checkvalidindex(L, o);
  api_check(L, ttistable(L->top - 1));
  if (isLfunction(o)) {
    res = 1;
    clvalue(o)->l.g = *(L->top - 1);
    luaC_objbarrier(L, clvalue(o), hvalue(L->top - 1));
  }
  L->top--;
  lua_unlock(L);
  return res;
}
