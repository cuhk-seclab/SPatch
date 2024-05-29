static void check (LexState *ls, int c) {
  if (ls->t.token != c)
    error_expected(ls, c);
}
