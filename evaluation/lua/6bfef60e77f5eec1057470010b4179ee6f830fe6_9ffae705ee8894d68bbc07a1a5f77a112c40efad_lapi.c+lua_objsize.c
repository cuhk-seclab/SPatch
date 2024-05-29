LUA_API size_t lua_objsize (lua_State *L, int idx) {
  StkId o = index2adr(L, idx);
  switch (ttype(o)) {
    case LUA_TSTRING: return tsvalue(o)->len;
    case LUA_TUSERDATA: return uvalue(o)->len;
    case LUA_TTABLE: return luaH_getn(hvalue(o));
    case LUA_TNUMBER: {
      size_t l;
      lua_lock(L);  /* `luaV_tostring' may create a new string */
      l = (luaV_tostring(L, o) ? tsvalue(o)->len : 0);
      lua_unlock(L);
      return l;
    }
    default: return 0;
  }
}
