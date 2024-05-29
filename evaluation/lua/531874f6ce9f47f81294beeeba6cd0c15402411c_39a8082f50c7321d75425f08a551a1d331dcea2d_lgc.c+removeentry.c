static void removeentry (Node *n) {
  setnilvalue(gval(n));  /* remove corresponding value ... */
  if (iscollectable(gkey(n)))
    setttype(gkey(n), LUA_TDEADKEY);  /* dead key; remove it */
}
