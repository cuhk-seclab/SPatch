static void new_localvar (LexState *ls, TString *name, int n) {
  FuncState *fs = ls->fs;
  luaY_checklimit(fs, fs->nactvar+n+1, LUAC_MAXVARS, "local variables");
  fs->actvar[fs->nactvar+n] = cast(unsigned short, registerlocalvar(ls, name));
}
