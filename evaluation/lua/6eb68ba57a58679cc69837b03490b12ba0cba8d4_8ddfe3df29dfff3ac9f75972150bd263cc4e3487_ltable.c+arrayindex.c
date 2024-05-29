static int arrayindex (const TValue *key) {
  if (ttisnumber(key)) {
    lua_Number n = nvalue(key);
    int k;
    lua_number2int(k, n);
    if (num_eq(cast(lua_Number, k), nvalue(key)))
      return k;
  }
  return -1;  /* `key' did not match some condition */
}
