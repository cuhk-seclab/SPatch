const TValue *luaH_getnum (Table *t, int key) {
  /* (1 <= key && key <= t->sizearray) */
  if (cast(unsigned int, key-1) < cast(unsigned int, t->sizearray))
    return &t->array[key-1];
  else {
    lua_Number nk = cast(lua_Number, key);
    Node *n = hashnum(t, nk);
    do {  /* check whether `key' is somewhere in the chain */
      if (ttisnumber(gkey(n)) && nvalue(gkey(n)) == nk)
        return gval(n);  /* that's it */
      else n = gnext(n);
    } while (n);
    return &luaO_nilobject;
  }
}
