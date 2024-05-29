static void callTM (lua_State *L, const TValue *f, const TValue *p1,                                const TValue *p2, const TValue *p3) {
  luaD_checkstack(L, 4);
  setobj2s(L, L->top, f);  /* push function */
  setobj2s(L, L->top+1, p1);  /* 1st argument */
  setobj2s(L, L->top+2, p2);  /* 2nd argument */
  setobj2s(L, L->top+3, p3);  /* 3th argument */
  L->top += 4;
  luaD_call(L, L->top - 4, 0);
}
