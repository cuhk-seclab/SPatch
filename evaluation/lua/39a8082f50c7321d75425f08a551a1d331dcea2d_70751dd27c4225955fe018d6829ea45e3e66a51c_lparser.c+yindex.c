static void yindex (LexState *ls, expdesc *v) {
  /* index -> '[' expr ']' */
  next(ls);  /* skip the '[' */
  expr(ls, v);
  luaK_exp2val(ls->fs, v);
  checknext(ls, ']');
}
