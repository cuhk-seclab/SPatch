static void rehash (lua_State *L, Table *t) {
  int nasize, nhsize;
  numuse(t, &nasize, &nhsize);  /* compute new sizes for array and hash parts */
  luaH_resize(L, t, nasize, luaO_log2(nhsize)+1);
}
