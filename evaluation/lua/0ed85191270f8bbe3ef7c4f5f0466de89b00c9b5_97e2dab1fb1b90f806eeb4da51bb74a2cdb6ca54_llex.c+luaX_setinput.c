void luaX_setinput (lua_State *L, LexState *ls, ZIO *z, TString *source) {
  ls->L = L;
  ls->lookahead.token = TK_EOS;  /* no look-ahead token */
  ls->z = z;
  ls->fs = NULL;
  ls->linenumber = 1;
  ls->lastline = 1;
  ls->source = source;
  luaZ_resizebuffer(ls->L, ls->buff, LUA_MINBUFFER);  /* initialize buffer */
  next(ls);  /* read first char */
}
