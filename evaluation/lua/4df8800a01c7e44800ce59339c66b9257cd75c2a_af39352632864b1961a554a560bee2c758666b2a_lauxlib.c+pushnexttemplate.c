static const char *pushnexttemplate (lua_State *L, const char *path) {
  const char *l;
  if (*path == '\0') return NULL;  /* no more templates */
  if (*path == LUA_PATH_SEP) path++;  /* skip separator */
  l = strchr(path, LUA_PATH_SEP);  /* find next separator */
  if (l == NULL) l = path+strlen(path);
  lua_pushlstring(L, path, l - path);  /* template */
  return l;
}
