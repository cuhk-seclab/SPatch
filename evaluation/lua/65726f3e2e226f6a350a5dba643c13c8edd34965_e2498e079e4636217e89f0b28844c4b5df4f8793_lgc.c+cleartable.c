static void cleartable (GCObject *l) {
  while (l) {
    Table *h = gco2h(l);
    int i = h->sizearray;
    lua_assert(testbit(h->marked, VALUEWEAKBIT) ||
               testbit(h->marked, KEYWEAKBIT));
    if (testbit(h->marked, VALUEWEAKBIT)) {
      while (i--) {
        TValue *o = &h->array[i];
        if (iscleared(o, 0))  /* value was collected? */
          setnilvalue(o);  /* remove value */
      }
    }
    i = sizenode(h);
    while (i--) {
      Node *n = gnode(h, i);
      if (!ttisnil(gval(n)) &&  /* non-empty entry? */
          (iscleared(key2tval(n), 1) || iscleared(gval(n), 0)))
        removeentry(n);  /* remove entry from table */
    }
    l = h->gclist;
  }
}
