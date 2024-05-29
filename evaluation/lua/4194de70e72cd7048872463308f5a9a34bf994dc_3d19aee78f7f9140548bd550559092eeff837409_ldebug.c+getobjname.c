static const char *getobjname (CallInfo *ci, int stackpos, const char **name) {
  if (isLua(ci)) {  /* a Lua function? */
    Proto *p = ci_func(ci)->l.p;
    int pc = currentpc(ci);
    Instruction i;
    *name = luaF_getlocalname(p, stackpos+1, pc);
    if (*name)  /* is a local? */
      return "local";
    i = symbexec(p, pc, stackpos);  /* try symbolic execution */
    lua_assert(pc != -1);
    switch (GET_OPCODE(i)) {
      case OP_GETGLOBAL: {
        int g = GETARG_Bx(i);  /* global index */
        lua_assert(ttisstring(&p->k[g]));
        *name = svalue(&p->k[g]);
        return "global";
      }
      case OP_MOVE: {
        int a = GETARG_A(i);
        int b = GETARG_B(i);  /* move from `b' to `a' */
        if (b < a)
          return getobjname(ci, b, name);  /* get name for `b' */
        break;
      }
      case OP_GETTABLE: {
        int k = GETARG_C(i);  /* key index */
        *name = kname(p, k);
        return "field";
      }
      case OP_GETUPVAL: {
        int u = GETARG_B(i);  /* upvalue index */
        *name = getstr(p->upvalues[u]);
        return "upvalue";
      }
      case OP_SELF: {
        int k = GETARG_C(i);  /* key index */
        *name = kname(p, k);
        return "method";
      }
      default: break;
    }
  }
  return NULL;  /* no useful name found */
}
