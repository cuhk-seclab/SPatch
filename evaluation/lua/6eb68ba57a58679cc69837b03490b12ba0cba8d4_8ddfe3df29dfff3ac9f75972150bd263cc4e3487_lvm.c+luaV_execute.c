StkId luaV_execute (lua_State *L, int nexeccalls) {
  LClosure *cl;
  TValue *k;
  StkId base;
  const Instruction *pc;
 callentry:  /* entry point when calling new functions */
  if (L->hookmask & LUA_MASKCALL)
    luaD_callhook(L, LUA_HOOKCALL, -1);
 retentry:  /* entry point when returning to old functions */
  pc = L->ci->savedpc;
  cl = &clvalue(L->ci->func)->l;
  base = L->base;
  k = cl->p->k;
  /* main loop of interpreter */
  for (;;) {
    const Instruction i = *pc++;
    StkId ra;
    if ((L->hookmask & (LUA_MASKLINE | LUA_MASKCOUNT)) &&
        (--L->hookcount == 0 || L->hookmask & LUA_MASKLINE)) {
      traceexec(L, pc);  /***/
      if (L->status == LUA_YIELD) {  /* did hook yield? */
        L->ci->savedpc = pc - 1;
        return NULL;
      }
      base = L->base;
    }
    /* warning!! several calls may realloc the stack and invalidate `ra' */
    ra = RA(i);
    lua_assert(base == L->ci->base && base == L->base);
    lua_assert(base <= L->top && L->top <= L->stack + L->stacksize);
    lua_assert(L->top == L->ci->top || luaG_checkopenop(i));
    switch (GET_OPCODE(i)) {
      case OP_MOVE: {
        setobjs2s(L, ra, RB(i));
        continue;
      }
      case OP_LOADK: {
        setobj2s(L, ra, KBx(i));
        continue;
      }
      case OP_LOADBOOL: {
        setbvalue(ra, GETARG_B(i));
        if (GETARG_C(i)) pc++;  /* skip next instruction (if C) */
        continue;
      }
      case OP_LOADNIL: {
        TValue *rb = RB(i);
        do {
          setnilvalue(rb--);
        } while (rb >= ra);
        continue;
      }
      case OP_GETUPVAL: {
        int b = GETARG_B(i);
        setobj2s(L, ra, cl->upvals[b]->v);
        continue;
      }
      case OP_GETGLOBAL: {
        TValue *rb = KBx(i);
        lua_assert(ttisstring(rb) && ttistable(&cl->g));
        base = luaV_gettable(L, &cl->g, rb, ra, pc);  /***/
        continue;
      }
      case OP_GETTABLE: {
        base = luaV_gettable(L, RB(i), RKC(i), ra, pc);  /***/
        continue;
      }
      case OP_SETGLOBAL: {
        lua_assert(ttisstring(KBx(i)) && ttistable(&cl->g));
        base = luaV_settable(L, &cl->g, KBx(i), ra, pc);  /***/
        continue;
      }
      case OP_SETUPVAL: {
        UpVal *uv = cl->upvals[GETARG_B(i)];
        setobj(L, uv->v, ra);
        luaC_barrier(L, uv, ra);
        continue;
      }
      case OP_SETTABLE: {
        base = luaV_settable(L, ra, RKB(i), RKC(i), pc);  /***/
        continue;
      }
      case OP_NEWTABLE: {
        int b = GETARG_B(i);
        int c = GETARG_C(i);
        sethvalue(L, ra, luaH_new(L, luaO_fb2int(b), luaO_fb2int(c)));
        L->ci->savedpc = pc;
        luaC_checkGC(L);  /***/
        base = L->base;
        continue;
      }
      case OP_SELF: {
        StkId rb = RB(i);
        setobjs2s(L, ra+1, rb);
        base = luaV_gettable(L, rb, RKC(i), ra, pc);  /***/
        continue;
      }
      case OP_ADD: {
        TValue *rb = RKB(i);
        TValue *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          setnvalue(ra, nvalue(rb) + nvalue(rc));
        }
        else
          base = Arith(L, ra, rb, rc, TM_ADD, pc);  /***/
        continue;
      }
      case OP_SUB: {
        TValue *rb = RKB(i);
        TValue *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          setnvalue(ra, nvalue(rb) - nvalue(rc));
        }
        else
          base = Arith(L, ra, rb, rc, TM_SUB, pc);  /***/
        continue;
      }
      case OP_MUL: {
        TValue *rb = RKB(i);
        TValue *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          setnvalue(ra, nvalue(rb) * nvalue(rc));
        }
        else
          base = Arith(L, ra, rb, rc, TM_MUL, pc);  /***/
        continue;
      }
      case OP_DIV: {
        TValue *rb = RKB(i);
        TValue *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          setnvalue(ra, nvalue(rb) / nvalue(rc));
        }
        else
          base = Arith(L, ra, rb, rc, TM_DIV, pc);  /***/
        continue;
      }
      case OP_POW: {
        TValue *rb = RKB(i);
        TValue *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          setnvalue(ra, lua_pow(nvalue(rb), nvalue(rc)));
        }
        else
          base = Arith(L, ra, rb, rc, TM_POW, pc);  /***/
        continue;
      }
      case OP_UNM: {
        const TValue *rb = RB(i);
        TValue temp;
        if (tonumber(rb, &temp)) {
          setnvalue(ra, -nvalue(rb));
        }
        else {
          setnilvalue(&temp);
          L->ci->savedpc = pc;
          if (!call_binTM(L, RB(i), &temp, ra, TM_UNM))  /***/
            luaG_aritherror(L, RB(i), &temp);
          base = L->base;
        }
        continue;
      }
      case OP_NOT: {
        int res = l_isfalse(RB(i));  /* next assignment may change this value */
        setbvalue(ra, res);
        continue;
      }
      case OP_CONCAT: {
        int b = GETARG_B(i);
        int c = GETARG_C(i);
        L->ci->savedpc = pc;
        luaV_concat(L, c-b+1, c);  /* may change `base' (and `ra') */  /***/
        luaC_checkGC(L);  /***/
        base = L->base;
        setobjs2s(L, RA(i), base+b);
        continue;
      }
      case OP_JMP: {
        dojump(L, pc, GETARG_sBx(i));
        continue;
      }
      case OP_EQ: {
        L->ci->savedpc = pc;
        if (equalobj(L, RKB(i), RKC(i)) != GETARG_A(i)) pc++;  /***/
        else dojump(L, pc, GETARG_sBx(*pc) + 1);
        base = L->base;
        continue;
      }
      case OP_LT: {
        L->ci->savedpc = pc;
        if (luaV_lessthan(L, RKB(i), RKC(i)) != GETARG_A(i)) pc++;  /***/
        else dojump(L, pc, GETARG_sBx(*pc) + 1);
        base = L->base;
        continue;
      }
      case OP_LE: {
        L->ci->savedpc = pc;
        if (lessequal(L, RKB(i), RKC(i)) != GETARG_A(i)) pc++;  /***/
        else dojump(L, pc, GETARG_sBx(*pc) + 1);
        base = L->base;
        continue;
      }
      case OP_TEST: {
        TValue *rb = RB(i);
        if (l_isfalse(rb) == GETARG_C(i)) pc++;
        else {
          setobjs2s(L, ra, rb);
          dojump(L, pc, GETARG_sBx(*pc) + 1);
        }
        continue;
      }
      case OP_CALL: {  /***/
        int pcr;
        int b = GETARG_B(i);
        int nresults = GETARG_C(i) - 1;
        if (b != 0) L->top = ra+b;  /* else previous instruction set top */
        L->ci->savedpc = pc;
        pcr = luaD_precall(L, ra, nresults);
        if (pcr == PCRLUA) {
          nexeccalls++;
          goto callentry;  /* restart luaV_execute over new Lua function */
        }
        else if (pcr == PCRC) {
          /* it was a C function (`precall' called it); adjust results */
          if (nresults >= 0) L->top = L->ci->top;
          base = L->base;
          continue;
        }
        else {
          lua_assert(pcr == PCRYIELD);
          return NULL;
        }
      }
      case OP_TAILCALL: {  /***/
        int pcr;
        int b = GETARG_B(i);
        if (b != 0) L->top = ra+b;  /* else previous instruction set top */
        L->ci->savedpc = pc;
        lua_assert(GETARG_C(i) - 1 == LUA_MULTRET);
        pcr = luaD_precall(L, ra, LUA_MULTRET);
        if (pcr == PCRLUA) {
          /* tail call: put new frame in place of previous one */
          CallInfo *ci = L->ci - 1;  /* previous frame */
          int aux;
          StkId func = ci->func;
          StkId pfunc = (ci+1)->func;  /* previous function index */
          base = ci->base = ci->func + ((ci+1)->base - pfunc);
          L->base = base;
          if (L->openupval) luaF_close(L, base);
          for (aux = 0; pfunc+aux < L->top; aux++)  /* move frame down */
            setobjs2s(L, func+aux, pfunc+aux);
          ci->top = L->top = func+aux;  /* correct top */
          lua_assert(L->top == L->base + clvalue(func)->l.p->maxstacksize);
          ci->savedpc = L->ci->savedpc;
          ci->tailcalls++;  /* one more call lost */
          L->ci--;  /* remove new frame */
          goto callentry;
        }
        else if (pcr == PCRC) {
          /* it was a C function (`precall' called it) */
          base = L->base;
          continue;
        }
        else {
          lua_assert(pcr == PCRYIELD);
          return NULL;
        }
      }
      case OP_RETURN: {
        CallInfo *ci = L->ci - 1;  /* previous function frame */
        int b = GETARG_B(i);
        if (b != 0) L->top = ra+b-1;
        if (L->openupval) luaF_close(L, base);
        L->ci->savedpc = pc;
        if (--nexeccalls == 0)  /* was previous function running `here'? */
          return ra;  /* no: return */
        else {  /* yes: continue its execution */
          int nresults = (ci+1)->nresults;
          lua_assert(isLua(ci));
          lua_assert(GET_OPCODE(*(ci->savedpc - 1)) == OP_CALL);
          luaD_poscall(L, nresults, ra);
          if (nresults >= 0) L->top = L->ci->top;
          goto retentry;
        }
      }
      case OP_FORLOOP: {
        lua_Number step = nvalue(ra+2);
        lua_Number idx = nvalue(ra) + step;  /* increment index */
        lua_Number limit = nvalue(ra+1);
        if (step > 0 ? idx <= limit : idx >= limit) {
          dojump(L, pc, GETARG_sBx(i));  /* jump back */
          setnvalue(ra, idx);  /* update internal index... */
          setnvalue(ra+3, idx);  /* ...and external index */
        }
        continue;
      }
      case OP_FORPREP: {  /***/
        const TValue *init = ra;
        const TValue *plimit = ra+1;
        const TValue *pstep = ra+2;
        L->ci->savedpc = pc;
        if (!tonumber(init, ra))
          luaG_runerror(L, "`for' initial value must be a number");
        else if (!tonumber(plimit, ra+1))
          luaG_runerror(L, "`for' limit must be a number");
        else if (!tonumber(pstep, ra+2))
          luaG_runerror(L, "`for' step must be a number");
        setnvalue(ra, nvalue(ra) - nvalue(pstep));
        dojump(L, pc, GETARG_sBx(i));
        continue;
      }
      case OP_TFORLOOP: {
        StkId cb = ra + 3;  /* call base */
        setobjs2s(L, cb+2, ra+2);
        setobjs2s(L, cb+1, ra+1);
        setobjs2s(L, cb, ra);
        L->top = cb+3;  /* func. + 2 args (state and index) */
        L->ci->savedpc = pc;
        luaD_call(L, cb, GETARG_C(i));  /***/
        L->top = L->ci->top;
        base = L->base;
        cb = RA(i) + 3;  /* previous call may change the stack */
        if (ttisnil(cb))  /* break loop? */
          pc++;  /* skip jump (break loop) */
        else {
          setobjs2s(L, cb-1, cb);  /* save control variable */
          dojump(L, pc, GETARG_sBx(*pc) + 1);  /* jump back */
        }
        continue;
      }
      case OP_TFORPREP: {  /* for compatibility only */
        if (ttistable(ra)) {
          setobjs2s(L, ra+1, ra);
          setobj2s(L, ra, luaH_getstr(hvalue(gt(L)), luaS_new(L, "next")));
        }
        dojump(L, pc, GETARG_sBx(i));
        continue;
      }
      case OP_SETLIST: {
        int n = GETARG_B(i);
        int c = GETARG_C(i);
        int last;
        Table *h;
        runtime_check(L, ttistable(ra));
        h = hvalue(ra);
        if (n == 0) {
          n = L->top - ra - 1;
          L->top = L->ci->top;
        }
        if (c == 0) c = cast(int, *pc++);
        last = ((c-1)*LFIELDS_PER_FLUSH) + n + LUA_FIRSTINDEX - 1;
        if (last > h->sizearray)  /* needs more space? */
          luaH_resizearray(L, h, last);  /* pre-alloc it at once */
        for (; n > 0; n--) {
          TValue *val = ra+n;
          setobj2t(L, luaH_setnum(L, h, last--), val);
          luaC_barriert(L, h, val);
        }
        continue;
      }
      case OP_CLOSE: {
        luaF_close(L, ra);
        continue;
      }
      case OP_CLOSURE: {
        Proto *p;
        Closure *ncl;
        int nup, j;
        p = cl->p->p[GETARG_Bx(i)];
        nup = p->nups;
        ncl = luaF_newLclosure(L, nup, &cl->g);
        ncl->l.p = p;
        for (j=0; j<nup; j++, pc++) {
          if (GET_OPCODE(*pc) == OP_GETUPVAL)
            ncl->l.upvals[j] = cl->upvals[GETARG_B(*pc)];
          else {
            lua_assert(GET_OPCODE(*pc) == OP_MOVE);
            ncl->l.upvals[j] = luaF_findupval(L, base + GETARG_B(*pc));
          }
        }
        setclvalue(L, ra, ncl);
        L->ci->savedpc = pc;
        luaC_checkGC(L);  /***/
        base = L->base;
        continue;
      }
      case OP_VARARG: {
        int b = GETARG_B(i) - 1;
        int j;
        CallInfo *ci = L->ci;
        int n = ci->base - ci->func - cl->p->numparams - 1;
        if (b == LUA_MULTRET) {
          b = n;
          L->top = ra + n;
        }
        for (j=0; j<b && j<n; j++)
          setobjs2s(L, ra+j, ci->base - n + j);
        for (; j<b; j++)
          setnilvalue(ra+j);
        continue;
      }
    }
  }
}
