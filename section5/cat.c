#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static void do_cat(int fd, const char* s);
static void die(const char* s);

int main(int argc, char* argv[]) {
  int i;
  if (argc < 2) {
    do_cat(STDIN_FILENO, "stdin");
    exit(1);
  }
  for (i = 1; i < argc; i++) {
    int fd = open(argv[i], O_RDONLY);
    if (fd < 0) die(argv[i]);
    do_cat(fd, argv[i]);
  }
  exit(0);
}

#define BUFFER_SIZE 2048
static void do_cat(int fd, const char* s) {
  unsigned char buf[BUFFER_SIZE];
  int n;

  while (1) {
    n = read(fd, buf, sizeof buf);
    if (n < 0) die(s);
    if (n == 0) break;
    if (write(STDOUT_FILENO, buf, n) < 0) die(s);
  }
  if (close(fd) < 0) die(s);
}

static void die(const char* s) {
  perror(s);
  exit(1);
}
