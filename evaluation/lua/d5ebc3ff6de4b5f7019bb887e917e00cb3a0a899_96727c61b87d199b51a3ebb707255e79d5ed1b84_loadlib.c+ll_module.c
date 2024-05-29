static int ll_module (lua_State *L) {
  const char *modname = luaL_checkstring(L, 1);
  const char *dot;
  lua_settop(L, 1);
  lua_getfield(L, LUA_REGISTRYINDEX, "_LOADED");
  /* try to find given table */
  luaL_getfield(L, LUA_GLOBALSINDEX, modname);
  if (lua_isnil(L, -1)) {  /* not found? */
    lua_pop(L, 1);  /* remove previous result */
    lua_newtable(L);  /* create it */
    /* register it with given name */
    lua_pushvalue(L, -1);
    luaL_setfield(L, LUA_GLOBALSINDEX, modname);
  }
  else if (!lua_istable(L, -1))
    return luaL_error(L, "name conflict for module `%s'", modname);
  /* check whether table already has a _NAME field */
  lua_getfield(L, -1, "_NAME");
  if (!lua_isnil(L, -1))  /* is table an initialized module? */
    lua_pop(L, 1);
  else {  /* no; initialize it */
    lua_pop(L, 1);
    lua_newtable(L);  /* create new metatable */
    lua_pushvalue(L, LUA_GLOBALSINDEX);
    lua_setfield(L, -2, "__index");  /* mt.__index = _G */
    lua_setmetatable(L, -2);
    lua_pushstring(L, modname);
    lua_setfield(L, -2, "_NAME");
    dot = strrchr(modname, '.');  /* look for last dot in module name */
    if (dot == NULL) dot = modname;
    else dot++;
    /* set _PACKAGE as package name (full module name minus last part) */
    lua_pushlstring(L, modname, dot - modname);
    lua_setfield(L, -2, "_PACKAGE");
  }
  lua_pushvalue(L, -1);
  lua_setfield(L, 2, modname);  /* _LOADED[modname] = new table */
  setfenv(L);
  return 0;
}
