StkId luaV_execute (lua_State *L, int nexeccalls) {
  LClosure *cl;
  StkId base;
  TValue *k;
  const Instruction *pc;
 callentry:  /* entry point when calling new functions */
  if (L->hookmask & LUA_MASKCALL)
    luaD_callhook(L, LUA_HOOKCALL, -1);
 retentry:  /* entry point when returning to old functions */
  pc = L->savedpc;
  cl = &clvalue(L->ci->func)->l;
  base = L->base;
  k = cl->p->k;
  /* main loop of interpreter */
  for (;;) {
    const Instruction i = *pc++;
    StkId ra;
    if ((L->hookmask & (LUA_MASKLINE | LUA_MASKCOUNT)) &&
        (--L->hookcount == 0 || L->hookmask & LUA_MASKLINE)) {
      traceexec(L, pc);
      if (L->status == LUA_YIELD) {  /* did hook yield? */
        L->savedpc = pc - 1;
        return NULL;
      }
      base = L->base;
    }
    /* warning!! several calls may realloc the stack and invalidate `ra' */
    ra = RA(i);
    lua_assert(base == L->base && L->base == L->ci->base);
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
        TValue g;
        TValue *rb = KBx(i);
        sethvalue(L, &g, cl->env);
        lua_assert(ttisstring(rb));
        Protect(luaV_gettable(L, &g, rb, ra));
        continue;
      }
      case OP_GETTABLE: {
        Protect(luaV_gettable(L, RB(i), RKC(i), ra));
        continue;
      }
      case OP_SETGLOBAL: {
        TValue g;
        sethvalue(L, &g, cl->env);
        lua_assert(ttisstring(KBx(i)));
        Protect(luaV_settable(L, &g, KBx(i), ra));
        continue;
      }
      case OP_SETUPVAL: {
        UpVal *uv = cl->upvals[GETARG_B(i)];
        setobj(L, uv->v, ra);
        luaC_barrier(L, uv, ra);
        continue;
      }
      case OP_SETTABLE: {
        Protect(luaV_settable(L, ra, RKB(i), RKC(i)));
        continue;
      }
      case OP_NEWTABLE: {
        int b = GETARG_B(i);
        int c = GETARG_C(i);
        sethvalue(L, ra, luaH_new(L, luaO_fb2int(b), luaO_fb2int(c)));
        Protect(luaC_checkGC(L));
        continue;
      }
      case OP_SELF: {
        StkId rb = RB(i);
        setobjs2s(L, ra+1, rb);
        Protect(luaV_gettable(L, rb, RKC(i), ra));
        continue;
      }
      case OP_ADD: {
        TValue *rb = RKB(i);
        TValue *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          lua_Number nb = nvalue(rb), nc = nvalue(rc);
          setnvalue(ra, luai_numadd(nb, nc));
        }
        else
          Protect(Arith(L, ra, rb, rc, TM_ADD));
        continue;
      }
      case OP_SUB: {
        TValue *rb = RKB(i);
        TValue *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          lua_Number nb = nvalue(rb), nc = nvalue(rc);
          setnvalue(ra, luai_numsub(nb, nc));
        }
        else
          Protect(Arith(L, ra, rb, rc, TM_SUB));
        continue;
      }
      case OP_MUL: {
        TValue *rb = RKB(i);
        TValue *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          lua_Number nb = nvalue(rb), nc = nvalue(rc);
          setnvalue(ra, luai_nummul(nb, nc));
        }
        else
          Protect(Arith(L, ra, rb, rc, TM_MUL));
        continue;
      }
      case OP_DIV: {
        TValue *rb = RKB(i);
        TValue *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          lua_Number nb = nvalue(rb), nc = nvalue(rc);
          setnvalue(ra, luai_numdiv(nb, nc));
        }
        else
          Protect(Arith(L, ra, rb, rc, TM_DIV));
        continue;
      }
      case OP_MOD: {
        TValue *rb = RKB(i);
        TValue *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          lua_Number nb = nvalue(rb), nc = nvalue(rc);
          setnvalue(ra, luai_nummod(nb, nc));
        }
        else
          Protect(Arith(L, ra, rb, rc, TM_MOD));
        continue;
      }
      case OP_POW: {
        TValue *rb = RKB(i);
        TValue *rc = RKC(i);
        if (ttisnumber(rb) && ttisnumber(rc)) {
          lua_Number nb = nvalue(rb), nc = nvalue(rc);
          setnvalue(ra, luai_numpow(nb, nc));
        }
        else
          Protect(Arith(L, ra, rb, rc, TM_POW));
        continue;
      }
      case OP_UNM: {
        const TValue *rb = RB(i);
        TValue temp;
        if (tonumber(rb, &temp)) {
          lua_Number nb = nvalue(rb);
          setnvalue(ra, luai_numunm(nb));
        }
        else {
          rb = RB(i);  /* `tonumber' erased `rb' */
          Protect(
            if (!call_binTM(L, rb, &luaO_nilobject, ra, TM_UNM))
              luaG_aritherror(L, rb, &luaO_nilobject);
          )
        }
        continue;
      }
      case OP_NOT: {
        int res = l_isfalse(RB(i));  /* next assignment may change this value */
        setbvalue(ra, res);
        continue;
      }
      case OP_SIZ: {
        const TValue *rb = RB(i);
        if (ttype(rb) == LUA_TTABLE) {
          setnvalue(ra, cast(lua_Number, luaH_getn(hvalue(rb))));
        }
        else {  /* try metamethod */
          Protect(
            if (!call_binTM(L, rb, &luaO_nilobject, ra, TM_SIZ))
              luaG_typeerror(L, rb, "get size of");
          )
        }
        continue;
      }
      case OP_CONCAT: {
        int b = GETARG_B(i);
        int c = GETARG_C(i);
        Protect(luaV_concat(L, c-b+1, c); luaC_checkGC(L));
        setobjs2s(L, RA(i), base+b);
        continue;
      }
      case OP_JMP: {
        dojump(L, pc, GETARG_sBx(i));
        continue;
      }
      case OP_EQ: {
        Protect(
          if (equalobj(L, RKB(i), RKC(i)) == GETARG_A(i))
            dojump(L, pc, GETARG_sBx(*pc));
        )
        pc++;
        continue;
      }
      case OP_LT: {
        Protect(
          if (luaV_lessthan(L, RKB(i), RKC(i)) == GETARG_A(i))
            dojump(L, pc, GETARG_sBx(*pc));
        )
        pc++;
        continue;
      }
      case OP_LE: {
        Protect(
          if (lessequal(L, RKB(i), RKC(i)) == GETARG_A(i))
            dojump(L, pc, GETARG_sBx(*pc));
        )
        pc++;
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
      case OP_CALL: {
        int b = GETARG_B(i);
        int nresults = GETARG_C(i) - 1;
        if (b != 0) L->top = ra+b;  /* else previous instruction set top */
        L->savedpc = pc;
        switch (luaD_precall(L, ra, nresults)) {
          case PCRLUA: {
            nexeccalls++;
            goto callentry;  /* restart luaV_execute over new Lua function */
          }
          case PCRC: {
            /* it was a C function (`precall' called it); adjust results */
            if (nresults >= 0) L->top = L->ci->top;
            base = L->base;
            continue;
          }
          default: {
            return NULL;
          }
        }
      }
      case OP_TAILCALL: {
        int b = GETARG_B(i);
        if (b != 0) L->top = ra+b;  /* else previous instruction set top */
        L->savedpc = pc;
        lua_assert(GETARG_C(i) - 1 == LUA_MULTRET);
        switch (luaD_precall(L, ra, LUA_MULTRET)) {
          case PCRLUA: {
            /* tail call: put new frame in place of previous one */
            CallInfo *ci = L->ci - 1;  /* previous frame */
            int aux;
            StkId func = ci->func;
            StkId pfunc = (ci+1)->func;  /* previous function index */
            if (L->openupval) luaF_close(L, ci->base);
            L->base = ci->base = ci->func + ((ci+1)->base - pfunc);
            for (aux = 0; pfunc+aux < L->top; aux++)  /* move frame down */
              setobjs2s(L, func+aux, pfunc+aux);
            ci->top = L->top = func+aux;  /* correct top */
            lua_assert(L->top == L->base + clvalue(func)->l.p->maxstacksize);
            ci->savedpc = L->savedpc;
            ci->tailcalls++;  /* one more call lost */
            L->ci--;  /* remove new frame */
            goto callentry;
          }
          case PCRC: {
            /* it was a C function (`precall' called it) */
            base = L->base;
            continue;
          }
          default: {
            return NULL;
          }
        }
      }
      case OP_RETURN: {
        int b = GETARG_B(i);
        if (b != 0) L->top = ra+b-1;
        if (L->openupval) luaF_close(L, base);
        L->savedpc = pc;
        if (--nexeccalls == 0)  /* was previous function running `here'? */
          return ra;  /* no: return */
        else {  /* yes: continue its execution */
          int nresults = L->ci->nresults;
          lua_assert(isLua(L->ci - 1));
          lua_assert(GET_OPCODE(*((L->ci - 1)->savedpc - 1)) == OP_CALL);
          luaD_poscall(L, nresults, ra);
          if (nresults >= 0) L->top = L->ci->top;
          goto retentry;
        }
      }
      case OP_FORLOOP: {
        lua_Number step = nvalue(ra+2);
        lua_Number idx = luai_numadd(nvalue(ra), step);  /* increment index */
        lua_Number limit = nvalue(ra+1);
        if (step > 0 ? luai_numle(idx, limit) : luai_numle(limit, idx)) {
          dojump(L, pc, GETARG_sBx(i));  /* jump back */
          setnvalue(ra, idx);  /* update internal index... */
          setnvalue(ra+3, idx);  /* ...and external index */
        }
        continue;
      }
      case OP_FORPREP: {
        const TValue *init = ra;
        const TValue *plimit = ra+1;
        const TValue *pstep = ra+2;
        L->savedpc = pc;  /* next steps may throw errors */
        if (!tonumber(init, ra))
          luaG_runerror(L, "`for' initial value must be a number");
        else if (!tonumber(plimit, ra+1))
          luaG_runerror(L, "`for' limit must be a number");
        else if (!tonumber(pstep, ra+2))
          luaG_runerror(L, "`for' step must be a number");
        setnvalue(ra, luai_numsub(nvalue(ra), nvalue(pstep)));
        dojump(L, pc, GETARG_sBx(i));
        continue;
      }
      case OP_TFORLOOP: {
        StkId cb = ra + 3;  /* call base */
        setobjs2s(L, cb+2, ra+2);
        setobjs2s(L, cb+1, ra+1);
        setobjs2s(L, cb, ra);
        L->top = cb+3;  /* func. + 2 args (state and index) */
        Protect(luaD_call(L, cb, GETARG_C(i)));
        L->top = L->ci->top;
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
        if (n == 0) {
          n = L->top - ra - 1;
          L->top = L->ci->top;
        }
        if (c == 0) c = cast(int, *pc++);
        runtime_check(L, ttistable(ra));
        h = hvalue(ra);
        last = ((c-1)*LFIELDS_PER_FLUSH) + n;
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
        ncl = luaF_newLclosure(L, nup, cl->env);
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
        Protect(luaC_checkGC(L));
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
