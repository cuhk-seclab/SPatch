static ptrdiff_t posrelat (ptrdiff_t pos, size_t len) {
  /* relative string position: negative means back from end */
  return (pos>=0) ? pos : (ptrdiff_t)len+pos+1;
}
