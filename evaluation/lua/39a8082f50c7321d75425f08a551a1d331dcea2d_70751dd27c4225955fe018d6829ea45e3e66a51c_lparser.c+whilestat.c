static void whilestat (LexState *ls, int line) {
  /* whilestat -> WHILE cond DO block END */
  Instruction codeexp[MAXEXPWHILE + EXTRAEXP];
  int lineexp;
  int i;
  int sizeexp;
  FuncState *fs = ls->fs;
  int whileinit, blockinit, expinit;
  expdesc v;
  BlockCnt bl;
  next(ls);  /* skip WHILE */
  whileinit = luaK_jump(fs);  /* jump to condition (which will be moved) */
  expinit = luaK_getlabel(fs);
  expr(ls, &v);  /* parse condition */
  if (v.k == VK) v.k = VTRUE;  /* `trues' are all equal here */
  lineexp = ls->linenumber;
  luaK_goiffalse(fs, &v);
  luaK_concat(fs, &v.f, fs->jpc);
  fs->jpc = NO_JUMP;
  sizeexp = fs->pc - expinit;  /* size of expression code */
  if (sizeexp > MAXEXPWHILE) 
    luaX_syntaxerror(ls, "`while' condition too complex");
  for (i = 0; i < sizeexp; i++)  /* save `exp' code */
    codeexp[i] = fs->f->code[expinit + i];
  fs->pc = expinit;  /* remove `exp' code */
  enterblock(fs, &bl, 1);
  checknext(ls, TK_DO);
  blockinit = luaK_getlabel(fs);
  block(ls);
  luaK_patchtohere(fs, whileinit);  /* initial jump jumps to here */
  /* move `exp' back to code */
  if (v.t != NO_JUMP) v.t += fs->pc - expinit;
  if (v.f != NO_JUMP) v.f += fs->pc - expinit;
  for (i=0; i<sizeexp; i++)
    luaK_code(fs, codeexp[i], lineexp);
  check_match(ls, TK_END, TK_WHILE, line);
  leaveblock(fs);
  luaK_patchlist(fs, v.t, blockinit);  /* true conditions go back to loop */
  luaK_patchtohere(fs, v.f);  /* false conditions finish the loop */
}
