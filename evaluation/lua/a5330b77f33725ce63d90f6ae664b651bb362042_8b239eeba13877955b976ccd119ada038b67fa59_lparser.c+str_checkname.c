static TString *str_checkname (LexState *ls) {
  TString *ts;
  if (ls->t.token != TK_NAME) error_expected(ls, TK_NAME);
  ts = ls->t.seminfo.ts;
  next(ls);
  return ts;
}
