static StkId Arith (lua_State *L, StkId ra, const TValue *rb,
                    const TValue *rc, TMS op, const Instruction *pc) {
  TValue tempb, tempc;
  const TValue *b, *c;
  L->ci->savedpc = pc;
  if ((b = luaV_tonumber(rb, &tempb)) != NULL &&
      (c = luaV_tonumber(rc, &tempc)) != NULL) {
    switch (op) {
      case TM_ADD: setnvalue(ra, nvalue(b) + nvalue(c)); break;
      case TM_SUB: setnvalue(ra, nvalue(b) - nvalue(c)); break;
      case TM_MUL: setnvalue(ra, nvalue(b) * nvalue(c)); break;
      case TM_DIV: setnvalue(ra, nvalue(b) / nvalue(c)); break;
      case TM_POW: setnvalue(ra, lua_pow(nvalue(b), nvalue(c))); break;
      default: lua_assert(0); break;
    }
  }
  else if (!call_binTM(L, rb, rc, ra, op))
    luaG_aritherror(L, rb, rc);
  return L->base;
}
