static void checkstacksizes (lua_State *L, StkId max) {
  int ci_used = L->ci - L->base_ci;  /* number of `ci' in use */
  int s_used = max - L->stack;  /* part of stack in use */
  if (L->size_ci > LUAC_MAXCALLS)  /* handling overflow? */
    return;  /* do not touch the stacks */
  if (4*ci_used < L->size_ci && 2*BASIC_CI_SIZE < L->size_ci)
    luaD_reallocCI(L, L->size_ci/2);  /* still big enough... */
  condhardstacktests(luaD_reallocCI(L, ci_used + 1));
  if (4*s_used < L->stacksize &&
      2*(BASIC_STACK_SIZE+EXTRA_STACK) < L->stacksize)
    luaD_reallocstack(L, L->stacksize/2);  /* still big enough... */
  condhardstacktests(luaD_reallocstack(L, s_used));
}
