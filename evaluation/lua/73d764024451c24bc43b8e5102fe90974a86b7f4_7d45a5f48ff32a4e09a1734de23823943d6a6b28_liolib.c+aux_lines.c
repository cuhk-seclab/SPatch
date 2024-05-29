static void aux_lines (lua_State *L, int idx, int close) {
  lua_pushvalue(L, idx);
  lua_pushboolean(L, close);  /* close/not close file when finished */
  lua_pushcclosure(L, io_readline, 2);
}
