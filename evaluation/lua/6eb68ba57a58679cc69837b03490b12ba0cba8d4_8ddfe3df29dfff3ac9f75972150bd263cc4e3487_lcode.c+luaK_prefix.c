void luaK_prefix (FuncState *fs, UnOpr op, expdesc *e) {
  if (op == OPR_MINUS) {
    luaK_exp2val(fs, e);
    if (e->k == VK && ttisnumber(&fs->f->k[e->info]))
      e->info = luaK_numberK(fs, num_unm(nvalue(&fs->f->k[e->info])));
    else {
      luaK_exp2anyreg(fs, e);
      freeexp(fs, e);
      e->info = luaK_codeABC(fs, OP_UNM, 0, e->info, 0);
      e->k = VRELOCABLE;
    }
  }
  else  /* op == NOT */
    codenot(fs, e);
}
