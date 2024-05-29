static int loadlib(lua_State *L, const char *path, const char *init)
{
 (void)path; (void)init; (void)&registerlib;  /* to avoid warnings */
 lua_pushliteral(L,"`loadlib' not supported");
 return ERR_ABSENT;
}
