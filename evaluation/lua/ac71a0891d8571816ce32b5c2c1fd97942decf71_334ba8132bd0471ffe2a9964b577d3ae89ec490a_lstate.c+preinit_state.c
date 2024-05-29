static void preinit_state (lua_State *L, global_State *g) {
  L->l_G = g;
  L->stack = NULL;
  L->stacksize = 0;
  L->errorJmp = NULL;
  L->hook = NULL;
  L->hookmask = 0;
  L->basehookcount = 0;
  L->allowhook = 1;
  resethookcount(L);
  L->openupval = NULL;
  L->size_ci = 0;
  L->nCcalls = 0;
  L->status = 0;
  L->base_ci = L->ci = NULL;
  L->errfunc = 0;
  setnilvalue(gt(L));
}
