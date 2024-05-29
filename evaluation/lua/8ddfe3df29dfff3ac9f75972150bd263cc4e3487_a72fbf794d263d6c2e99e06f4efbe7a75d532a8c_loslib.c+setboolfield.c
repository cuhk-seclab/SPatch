static void setboolfield (lua_State *L, const char *key, int value) {
  lua_pushboolean(L, value);
  lua_setfield(L, -2, key);
}
