#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static const size_t INIT_SIZE = 1;

int main(int argc, char* argv[]) {
  size_t size = INIT_SIZE;
  char* buf = malloc(size);
  if (!buf) exit(1);

  for (;;) {
    errno = 0;
    if (getcwd(buf, size)) break;
    if (errno != ERANGE) {
      perror("getcwd");
      exit(1);
    }
    size *= 2;
    printf("size = %ld\n", size);
    char* temp = realloc(buf, size);
    if (!temp) exit(1);
    buf = temp;
  }
  printf("current working directory is \"%s\"\n", buf);
  free(buf);
}
