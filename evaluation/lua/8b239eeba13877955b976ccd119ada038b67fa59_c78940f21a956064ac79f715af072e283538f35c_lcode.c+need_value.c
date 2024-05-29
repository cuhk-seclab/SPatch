static int need_value (FuncState *fs, int list, int cond) {
  for (; list != NO_JUMP; list = luaK_getjump(fs, list)) {
    Instruction i = *getjumpcontrol(fs, list);
    if (GET_OPCODE(i) != OP_TEST || GETARG_C(i) != cond) return 1;
  }
  return 0;  /* not found */
}
