const char *luaX_token2str (LexState *ls, int token) {
  if (token < FIRST_RESERVED) {
    lua_assert(token == cast(unsigned char, token));
    return (iscntrl(token)) ? luaO_pushfstring(ls->L, "char(%d)", token) :
                              luaO_pushfstring(ls->L, "%c", token);
  }
  else
    return token2string[token-FIRST_RESERVED];
}
