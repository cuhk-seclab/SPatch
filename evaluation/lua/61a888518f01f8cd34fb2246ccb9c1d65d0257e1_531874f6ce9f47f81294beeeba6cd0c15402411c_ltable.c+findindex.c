static int findindex (lua_State *L, Table *t, StkId key) {
  int i;
  if (ttisnil(key)) return -1;  /* first iteration */
  i = arrayindex(key);
  if (0 < i && i <= t->sizearray)  /* is `key' inside array part? */
    return i-1;  /* yes; that's the index (corrected to C) */
  else {
    const TValue *v = luaH_get(t, key);
    if (v == &luaO_nilobject)
      luaG_runerror(L, "invalid key for `next'");
    i = cast(int, (cast(const lu_byte *, v) -
                   cast(const lu_byte *, gval(gnode(t, 0)))) / sizeof(Node));
    return i + t->sizearray;  /* hash elements are numbered after array ones */
  }
}
