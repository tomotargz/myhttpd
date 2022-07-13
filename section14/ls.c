#include <dirent.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

static void do_ls(const char* path);

int main(int argc, char* argv[]) {
  if (argc == 1) {
    do_ls(".");
  } else {
    for (int i = 1; i < argc; ++i) {
      do_ls(argv[i]);
    }
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
    printf("%s\t", e->d_name);
    struct stat st;
    if (stat(e->d_name, &st) == -1) {
      fprintf(stderr, "stat failed\n");
    }
    char* owner = getpwuid(st.st_uid)->pw_name;
    char* last_modified = asctime(localtime(&st.st_mtime));
    printf("%s\t", owner);
    printf("%s", last_modified);
  }
  closedir(d);
}
