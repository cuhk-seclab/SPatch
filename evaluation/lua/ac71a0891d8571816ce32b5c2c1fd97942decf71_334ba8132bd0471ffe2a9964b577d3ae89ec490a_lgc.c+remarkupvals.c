static void remarkupvals (global_State *g) {
  GCObject *o;
  for (o = obj2gco(g->mainthread); o; o = o->gch.next) {
    lua_assert(!isblack(o));
    if (iswhite(o)) {
      GCObject *curr;
      for (curr = gco2th(o)->openupval; curr != NULL; curr = curr->gch.next) {
        if (isgray(curr)) {
          UpVal *uv = gco2uv(curr);
          markvalue(g, uv->v);
        }
      }
    }
  }
}
