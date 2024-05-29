static int loadline (lua_State *L) {
  int status;
  lua_settop(L, 0);
  if (!pushline(L, 1))
    return -1;  /* no input */
  for (;;) {  /* repeat until gets a complete line */
    status = luaL_loadbuffer(L, lua_tostring(L, 1), lua_strlen(L, 1), "=stdin");
    if (!incomplete(L, status)) break;  /* cannot try to add lines? */
    if (lua_readline(L, get_prompt(L, 0)) == 0)  /* no more input? */
      return -1;
    lua_insert(L, -2);  /* ...between the two lines */
    lua_concat(L, lua_gettop(L));  /* join lines */
  }
  lua_saveline(L, 1);
  lua_remove(L, 1);  /* remove line */
  return status;
}
