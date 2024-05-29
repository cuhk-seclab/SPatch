void luaX_init (lua_State *L) {
  int i;
  for (i=0; i<NUM_RESERVED; i++) {
    TString *ts = luaS_new(L, token2string[i]);
    luaS_fix(ts);  /* reserved words are never collected */
    lua_assert(strlen(token2string[i])+1 <= TOKEN_LEN);
    ts->tsv.reserved = cast(lu_byte, i+1);  /* reserved word */
  }
}
