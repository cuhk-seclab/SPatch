int luaG_checkcode (const Proto *pt) {
  return (symbexec(pt, pt->sizecode, NO_REG) != 0);
}
