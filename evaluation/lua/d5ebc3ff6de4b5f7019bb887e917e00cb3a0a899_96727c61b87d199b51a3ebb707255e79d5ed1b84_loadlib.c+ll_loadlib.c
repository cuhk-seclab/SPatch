static int ll_loadlib (lua_State *L) {
 static const char *const errcodes[] = {NULL, "open", "init", "absent"};
 const char *path=luaL_checkstring(L,1);
 const char *init=luaL_checkstring(L,2);
 int res = loadlib(L,path,init);
 if (res == 0)  /* no error? */
   return 1;  /* function is at stack top */
 else {  /* error */
  lua_pushnil(L);
  lua_insert(L,-2);  /* insert nil before error message */
  lua_pushstring(L, errcodes[res]);
  return 3;
 }
}
