LUA_API int lua_checkstack (lua_State *L, int size) {
  int res;
  lua_lock(L);
  if ((L->top - L->base + size) > MAXCSTACK)
    res = 0;  /* stack overflow */
  else {
    luaD_checkstack(L, size);
    if (L->ci->top < L->top + size)
      L->ci->top = L->top + size;
    res = 1;
  }
  lua_unlock(L);
  return res;
}
