LUALIB_API const char *luaL_searchpath (lua_State *L, const char *name,
                                                      const char *path) {
  FILE *f;
  const char *p = path;
  for (;;) {
    const char *fname;
    if ((p = pushnexttemplate(L, p)) == NULL) {
      lua_pushfstring(L, "no readable `%s' in path `%s'", name, path);
      return NULL;
    }
    fname = luaL_gsub(L, lua_tostring(L, -1), LUA_PATH_MARK, name);
    lua_remove(L, -2);  /* remove path template */
    f = fopen(fname, "r");  /* try to open it */
    if (f) {
      int err;
      getc(f);  /* try to read file */
      err = ferror(f);
      fclose(f);
      if (err == 0)  /* open and read sucessful? */
        return fname;  /* return that file name */
    }
    lua_pop(L, 1);  /* remove file name */ 
  }
}
