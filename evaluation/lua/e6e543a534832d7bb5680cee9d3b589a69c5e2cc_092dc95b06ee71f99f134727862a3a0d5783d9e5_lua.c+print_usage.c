static void print_usage (void) {
  fprintf(stderr,
  "usage: %s [options] [script [args]].\n"
  "Available options are:\n"
  "  -        execute stdin as a file\n"
  "  -e stat  execute string `stat'\n"
  "  -i       enter interactive mode after executing `script'\n"
  "  -l name  require library `name'\n"
  "  -v       show version information\n"
  "  -w	      control access to undefined globals\n"
  "  --       stop handling options\n" ,
  progname);
}
