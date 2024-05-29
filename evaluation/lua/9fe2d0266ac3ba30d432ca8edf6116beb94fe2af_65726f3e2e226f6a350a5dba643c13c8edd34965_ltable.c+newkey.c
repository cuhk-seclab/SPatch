static TValue *newkey (lua_State *L, Table *t, const TValue *key) {
  TValue *val;
  Node *mp = luaH_mainposition(t, key);
  if (!ttisnil(gval(mp))) {  /* main position is not free? */
    /* `mp' of colliding node */
    Node *othern = luaH_mainposition(t, key2tval(mp));
    Node *n = t->firstfree;  /* get a free place */
    if (othern != mp) {  /* is colliding node out of its main position? */
      /* yes; move colliding node into free position */
      while (gnext(othern) != mp) othern = gnext(othern);  /* find previous */
      gnext(othern) = n;  /* redo the chain with `n' in place of `mp' */
      *n = *mp;  /* copy colliding node into free pos. (mp->next also goes) */
      gnext(mp) = NULL;  /* now `mp' is free */
      setnilvalue(gval(mp));
    }
    else {  /* colliding node is in its own main position */
      /* new node will go into free position */
      gnext(n) = gnext(mp);  /* chain new position */
      gnext(mp) = n;
      mp = n;
    }
  }
  gkey(mp)->value = key->value; gkey(mp)->tt = key->tt;
  luaC_barriert(L, t, key);
  lua_assert(ttisnil(gval(mp)));
  for (;;) {  /* correct `firstfree' */
    if (ttisnil(gkey(t->firstfree)))
      return gval(mp);  /* OK; table still has a free place */
    else if (t->firstfree == t->node) break;  /* cannot decrement from here */
    else (t->firstfree)--;
  }
  /* no more free places; must create one */
  setbvalue(gval(mp), 0);  /* avoid new key being removed */
  rehash(L, t);  /* grow table */
  val = cast(TValue *, luaH_get(t, key));  /* get new position */
  lua_assert(ttisboolean(val));
  setnilvalue(val);
  return val;
}
