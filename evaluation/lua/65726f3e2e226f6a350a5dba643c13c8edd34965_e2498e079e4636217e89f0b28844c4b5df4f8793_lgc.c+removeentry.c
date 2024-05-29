static void removeentry (Node *n) {
  lua_assert(ttisnil(gval(n)));
  if (iscollectable(gkey(n)))
    setttype(gkey(n), LUA_TDEADKEY);  /* dead key; remove it */
}
