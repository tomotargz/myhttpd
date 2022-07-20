#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static void log_exit(char* fmt, ...);
static void* xmalloc(size_t s);

static void log_exit(char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
  exit(1);
}

static void* xmalloc(size_t s) {
  void* p = malloc(s);
  if (!p) log_exit("failed to allocate memory");
  return p;
}
