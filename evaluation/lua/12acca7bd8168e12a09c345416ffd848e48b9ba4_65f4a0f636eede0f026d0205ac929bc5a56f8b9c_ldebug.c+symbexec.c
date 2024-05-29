static Instruction symbexec (const Proto *pt, int lastpc, int reg) {
  int pc;
  int last;  /* stores position of last instruction that changed `reg' */
  last = pt->sizecode-1;  /* points to final return (a `neutral' instruction) */
  check(precheck(pt));
  for (pc = 0; pc < lastpc; pc++) {
    const Instruction i = pt->code[pc];
    OpCode op = GET_OPCODE(i);
    int a = GETARG_A(i);
    int b = 0;
    int c = 0;
    check(op < NUM_OPCODES);
    checkreg(pt, a);
    switch (getOpMode(op)) {
      case iABC: {
        b = GETARG_B(i);
        c = GETARG_C(i);
        check(checkArgMode(pt, b, getBMode(op)));
        check(checkArgMode(pt, c, getCMode(op)));
        break;
      }
      case iABx: {
        b = GETARG_Bx(i);
        if (getBMode(op) == OpArgK) check(b < pt->sizek);
        break;
      }
      case iAsBx: {
        b = GETARG_sBx(i);
        if (getBMode(op) == OpArgR) {
          int dest = pc+1+b;
          check(0 <= dest && dest < pt->sizecode);
          if (dest > 0) {
            /* cannot jump to a setlist count */
            const Instruction d = pt->code[dest-1];
            check(!(GET_OPCODE(d) == OP_SETLIST && GETARG_C(d) == 0));
          }
        }
        break;
      }
    }
    if (testAMode(op)) {
      if (a == reg) last = pc;  /* change register `a' */
    }
    if (testTMode(op)) {
      check(pc+2 < pt->sizecode);  /* check skip */
      check(GET_OPCODE(pt->code[pc+1]) == OP_JMP);
    }
    switch (op) {
      case OP_LOADBOOL: {
        check(c == 0 || pc+2 < pt->sizecode);  /* check its jump */
        break;
      }
      case OP_LOADNIL: {
        if (a <= reg && reg <= b)
          last = pc;  /* set registers from `a' to `b' */
        break;
      }
      case OP_GETUPVAL:
      case OP_SETUPVAL: {
        check(b < pt->nups);
        break;
      }
      case OP_GETGLOBAL:
      case OP_SETGLOBAL: {
        check(ttisstring(&pt->k[b]));
        break;
      }
      case OP_SELF: {
        checkreg(pt, a+1);
        if (reg == a+1) last = pc;
        break;
      }
      case OP_CONCAT: {
        check(b < c);  /* at least two operands */
        break;
      }
      case OP_TFORLOOP: {
        check(c >= 1);  /* at least one result (control variable) */
        checkreg(pt, a+3+c);  /* space for results */
        if (reg >= a+3) last = pc;  /* affect all regs above its call base */
        break;
      }
      case OP_TFORPREP:
      case OP_FORLOOP:
      case OP_FORPREP:
        checkreg(pt, a+3);
        /* go through */
      case OP_JMP: {
        int dest = pc+1+b;
        /* not full check and jump is forward and do not skip `lastpc'? */
        if (reg != NO_REG && pc < dest && dest <= lastpc)
          pc += b;  /* do the jump */
        break;
      }
      case OP_CALL:
      case OP_TAILCALL: {
        if (b != 0) {
          checkreg(pt, a+b-1);
        }
        c--;  /* c = num. returns */
        if (c == LUA_MULTRET) {
          check(checkopenop(pt, pc));
        }
        else if (c != 0)
          checkreg(pt, a+c-1);
        if (reg >= a) last = pc;  /* affect all registers above base */
        break;
      }
      case OP_RETURN: {
        b--;  /* b = num. returns */
        if (b > 0) checkreg(pt, a+b-1);
        break;
      }
      case OP_SETLIST: {
        if (b > 0) checkreg(pt, a + b);
        if (c == 0) pc++;
        break;
      }
      case OP_CLOSURE: {
        int nup;
        check(b < pt->sizep);
        nup = pt->p[b]->nups;
        check(pc + nup < pt->sizecode);
        for (; nup>0; nup--) {
          OpCode op1 = GET_OPCODE(pt->code[pc+nup]);
          check(op1 == OP_GETUPVAL || op1 == OP_MOVE);
        }
        break;
      }
      case OP_VARARG: {
        check(pt->is_vararg & NEWSTYLEVARARG);
        b--;
        if (b == LUA_MULTRET) check(checkopenop(pt, pc));
        checkreg(pt, a+b-1);
        break;
      }
      default: break;
    }
  }
  return pt->code[last];
}
