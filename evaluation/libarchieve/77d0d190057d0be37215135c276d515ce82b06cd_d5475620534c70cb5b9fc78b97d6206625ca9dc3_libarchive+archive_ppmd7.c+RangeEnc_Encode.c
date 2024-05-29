static void RangeEnc_Encode(CPpmd7z_RangeEnc *p, UInt32 start, UInt32 size, UInt32 total)
{
  p->Low += (UInt64)start * (UInt64)(p->Range /= total);
  p->Range *= size;
  while (p->Range < kTopValue)
  {
    p->Range <<= 8;
    RangeEnc_ShiftLow(p);
  }
}
