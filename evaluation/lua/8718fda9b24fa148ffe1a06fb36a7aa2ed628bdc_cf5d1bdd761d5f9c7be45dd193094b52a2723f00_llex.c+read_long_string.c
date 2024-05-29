static void read_long_string (LexState *ls, SemInfo *seminfo, int sep) {
  int cont = 0;
  save_and_next(ls);  /* skip 2nd `[' */
  if (currIsNewline(ls))  /* string starts with a newline? */
    inclinenumber(ls);  /* skip it */
  for (;;) {
    switch (ls->current) {
      case EOZ:
        luaX_lexerror(ls, (seminfo) ? "unfinished long string" :
                                   "unfinished long comment", TK_EOS);
        break;  /* to avoid warnings */
      case '[':
        if (skip_sep(ls) == sep) {
          save_and_next(ls);  /* skip 2nd `[' */
          cont++;
        }
        continue;
      case ']':
        if (skip_sep(ls) == sep) {
          save_and_next(ls);  /* skip 2nd `]' */
          if (cont-- == 0) goto endloop;
        }
        continue;
      case '\n':
      case '\r':
        save(ls, '\n');
        inclinenumber(ls);
        if (!seminfo) luaZ_resetbuffer(ls->buff);  /* avoid wasting space */
        continue;
      default:
        if (seminfo) save_and_next(ls);
        else next(ls);
    }
  } endloop:
  if (seminfo)
    seminfo->ts = luaX_newstring(ls, luaZ_buffer(ls->buff) + (2 + sep),
                                     luaZ_bufflen(ls->buff) - 2*(2 + sep));
}
