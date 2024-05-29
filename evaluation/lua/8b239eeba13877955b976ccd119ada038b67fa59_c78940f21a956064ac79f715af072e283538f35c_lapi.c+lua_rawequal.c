LUA_API int lua_rawequal (lua_State *L, int index1, int index2) {
  StkId o1 = index2adr(L, index1);
  StkId o2 = index2adr(L, index2);
  return (o1 == &luaO_nilobject || o2 == &luaO_nilobject) ? 0
         : luaO_rawequalObj(o1, o2);
}
