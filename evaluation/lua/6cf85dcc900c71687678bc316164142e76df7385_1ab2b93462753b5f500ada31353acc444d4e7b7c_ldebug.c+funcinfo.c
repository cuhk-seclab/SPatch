static void funcinfo (lua_Debug *ar, StkId func) {
  Closure *cl = clvalue(func);
  if (cl->c.isC) {
    ar->source = "=[C]";
    ar->linedefined = -1;
    ar->what = "C";
  }
  else {
    ar->source = getstr(cl->l.p->source);
    ar->linedefined = cl->l.p->lineDefined;
    ar->what = (ar->linedefined == 0) ? "main" : "Lua";
  }
  luaO_chunkid(ar->short_src, ar->source, LUA_IDSIZE);
}
