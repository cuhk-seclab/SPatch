LUA_API int lua_equal (lua_State *L, int index1, int index2) {
  StkId o1, o2;
  int i;
  lua_lock(L);  /* may call tag method */
  o1 = index2adr(L, index1);
  o2 = index2adr(L, index2);
  i = (o1 == &luaO_nilobject || o2 == &luaO_nilobject) ? 0
       : equalobj(L, o1, o2);
  lua_unlock(L);
  return i;
}
