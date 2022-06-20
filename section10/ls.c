#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

static void do_ls(const char* path);

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "%s: no arguments\n", argv[0]);
    exit(1);
  }
  for (int i = 1; i < argc; ++i) {
    do_ls(argv[i]);
  }
  exit(0);
}

void do_ls(const char* path) {
  DIR* d = opendir(path);
  if (!d) {
    perror(path);
    exit(1);
  }
  struct dirent* e;
  while ((e = readdir(d))) {
    printf("%s\n", e->d_name);
  }
  closedir(d);
}
