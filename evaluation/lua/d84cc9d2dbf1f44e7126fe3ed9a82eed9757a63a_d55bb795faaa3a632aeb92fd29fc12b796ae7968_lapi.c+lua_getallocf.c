LUA_API lua_Alloc lua_getallocf (lua_State *L, void **ud) {
  *ud = G(L)->ud;
  return G(L)->realloc;
}
