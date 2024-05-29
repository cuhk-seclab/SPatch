static void codebinop (FuncState *fs, expdesc *res, BinOpr op,
                       int o1, int o2) {
  if (op <= OPR_POW) {  /* arithmetic operator? */
    OpCode opc = cast(OpCode, (op - OPR_ADD) + OP_ADD);  /* ORDER OP */
    res->info = luaK_codeABC(fs, opc, 0, o1, o2);
    res->k = VRELOCABLE;
  }
  else {  /* test operator */
    static const OpCode ops[] = {OP_EQ, OP_EQ, OP_LT, OP_LE, OP_LT, OP_LE};
    int cond = 1;
    if (op >= OPR_GT) {  /* `>' or `>='? */
      int temp;  /* exchange args and replace by `<' or `<=' */
      temp = o1; o1 = o2; o2 = temp;  /* o1 <==> o2 */
    }
    else if (op == OPR_NE) cond = 0;
    res->info = condjump(fs, ops[op - OPR_NE], cond, o1, o2);
    res->k = VJMP;
  }
}
