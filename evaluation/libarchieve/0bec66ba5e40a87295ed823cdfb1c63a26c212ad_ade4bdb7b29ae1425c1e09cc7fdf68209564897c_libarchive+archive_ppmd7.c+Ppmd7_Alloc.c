static Bool Ppmd7_Alloc(CPpmd7 *p, UInt32 size)
{
  if (p->Base == 0 || p->Size != size)
  {
    /* RestartModel() below assumes that p->Size >= UNIT_SIZE
       (see the calculation of m->MinContext). */
    if (size < UNIT_SIZE) {
      return False;
    }
    Ppmd7_Free(p);
    p->AlignOffset =
      #ifdef PPMD_32BIT
        (4 - size) & 3;
      #else
        4 - (size & 3);
      #endif
    if ((p->Base = (Byte *)malloc(p->AlignOffset + size
        #ifndef PPMD_32BIT
        + UNIT_SIZE
        #endif
        )) == 0)
      return False;
    p->Size = size;
  }
  return True;
}
