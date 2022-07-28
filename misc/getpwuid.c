#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
  struct passwd* p = getpwnam("tomota");
  if (!p) {
    perror("getpwnam");
    exit(1);
  }
  printf("name: %s\n", p->pw_name);
  printf("passwd: %s\n", p->pw_passwd);
  printf("uid: %u\n", p->pw_uid);
  printf("gid: %u\n", p->pw_gid);
  printf("gecos: %s\n", p->pw_gecos);
  printf("dir: %s\n", p->pw_dir);
  printf("shell: %s\n", p->pw_shell);
}
