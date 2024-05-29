static const char *generic_reader (lua_State *L, void *ud, size_t *size) {
  (void)ud;  /* to avoid warnings */
  luaL_checkstack(L, 2, "too many nested functions");
  lua_pushvalue(L, 1);  /* get function */
  lua_call(L, 0, 1);  /* call it */
  if (lua_isnil(L, -1)) {
    *size = 0;
    return NULL;
  }
  else if (lua_isstring(L, -1)) {
    const char *res;
    lua_replace(L, 3);  /* save string in a reserved stack slot */
    res = lua_tostring(L, 3);
    return res;
  }
  else luaL_error(L, "reader function must return a string");
  return NULL;  /* to avoid warnings */
}
