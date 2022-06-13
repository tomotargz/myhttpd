#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int do_wc(int fd, const char* s);
static void die(const char* s);

int main(int argc, char* argv[]) {
  int i;
  long lc = 0;
  if (argc < 2) {
    lc += do_wc(STDIN_FILENO, "stdin");
  } else {
    for (i = 1; i < argc; i++) {
      int fd = open(argv[i], O_RDONLY);
      if (fd < 0) die(argv[i]);
      lc += do_wc(fd, argv[i]);
    }
  }
  printf("%ld\n", lc);
  exit(0);
}

#define BUFFER_SIZE 2048
static int do_wc(int fd, const char* s) {
  unsigned char buf[BUFFER_SIZE];
  int n, i, lc;
  lc = 0;

  while (1) {
    n = read(fd, buf, sizeof buf);
    if (n < 0) die(s);
    if (n == 0) break;
    for (i = 0; i < n; ++i) {
      if (buf[i] == '\n') ++lc;
    }
  }
  if (close(fd) < 0) die(s);
  return lc;
}

static void die(const char* s) {
  perror(s);
  exit(1);
}
