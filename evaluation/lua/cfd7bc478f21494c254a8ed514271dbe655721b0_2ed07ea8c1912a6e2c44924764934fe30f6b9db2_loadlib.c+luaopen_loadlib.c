LUALIB_API int luaopen_loadlib (lua_State *L)
{
 luaL_newmetatable(L, "_LOADLIB");
 lua_pushcfunction(L, gctm);
 lua_setfield(L, -2, "__gc");
 lua_register(L,"loadlib",loadlib);
 return 0;
}
