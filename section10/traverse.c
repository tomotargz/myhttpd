#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static void traverse(const char* path);

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s FILE\n", argv[0]);
    exit(1);
  }
  traverse(argv[1]);
}

void traverse(const char* path) {
  DIR* d = opendir(path);
  if (!d) {
    perror(path);
    exit(1);
  }
  struct dirent* e;
  while ((e = readdir(d))) {
    char fullpath[1024];
    sprintf(fullpath, "%s/%s", path, e->d_name);

    if (e->d_type == DT_REG) {
      printf("%s\n", fullpath);
    } else if (e->d_type == DT_DIR) {
      if (strcmp(e->d_name, ".") == 0) continue;
      if (strcmp(e->d_name, "..") == 0) continue;
      traverse(fullpath);
    } else if (e->d_type == DT_LNK) {
      struct stat st;
      stat(fullpath, &st);
      if (!S_ISREG(st.st_mode)) continue;
      printf("%s\n", fullpath);
    }
  }
  closedir(d);
}
