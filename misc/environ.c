#include <stdio.h>

extern char** environ;

int main(int argc, char* argv[]) {
  char** p = environ;
  while (*p) {
    printf("%s\n", *p);
    ++p;
  }
  return 0;
}
