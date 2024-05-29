void luaD_throw (lua_State *L, int errcode) {
  if (L->errorJmp) {
    L->errorJmp->status = errcode;
    L_THROW(L->errorJmp);
  }
  else {
    if (G(L)->panic) G(L)->panic(L);
    exit(EXIT_FAILURE);
  }
}
