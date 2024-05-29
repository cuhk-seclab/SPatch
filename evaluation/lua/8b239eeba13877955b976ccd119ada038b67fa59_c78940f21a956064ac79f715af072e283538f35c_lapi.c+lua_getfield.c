LUA_API void lua_getfield (lua_State *L, int idx, const char *k) {
  StkId t;
  TValue key;
  lua_lock(L);
  t = index2adr(L, idx);
  api_checkvalidindex(L, t);
  setsvalue(L, &key, luaS_new(L, k));
  luaV_gettable(L, t, &key, L->top, NULL);
  api_incr_top(L);
  lua_unlock(L);
}
