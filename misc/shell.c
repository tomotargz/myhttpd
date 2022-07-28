#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static int read_cmd(char* buf, int size);
static void exec_cmd(const char* buf);

int main(int argc, char* argv[]) {
  while (1) {
    printf("$ ");
    char cmd[1024];
    if (read_cmd(cmd, sizeof cmd) != -1) {
      exec_cmd(cmd);
    }
  }
}

int read_cmd(char* cmd, int n) {
  if (!fgets(cmd, n, stdin)) {
    fprintf(stderr, "fgets failed\n");
    return -1;
  }
  size_t len = strlen(cmd);
  if (cmd[len - 1] != '\n') {
    fprintf(stderr, "Command too long\n");
    while (fgetc(stdin) != '\n') {
    }
    return -1;
  }
  cmd[len - 1] = '\0';
  return 0;
}

void exec_cmd(const char* cmd) {
  char* argv[10];
  int argc = 0;
  for (int i = 0; i < strlen(cmd); ++i) {
    if (cmd[i] == ' ') continue;
    char buf[1024];
    int j = 0;
    while (i < strlen(cmd) && cmd[i] != ' ') {
      buf[j] = cmd[i];
      ++i;
      ++j;
    }
    buf[j] = '\0';
    argv[argc] = malloc(strlen(buf) + 1);
    memcpy(argv[argc], buf, strlen(buf) + 1);
    ++argc;
  }
  argv[argc] = NULL;

  pid_t pid = fork();
  if (pid < 0) {
    perror(argv[0]);
  } else if (pid == 0) {
    execvp(argv[0], argv);
    perror(argv[0]);
  } else {
    int status;
    waitpid(pid, &status, 0);
    for (int i = 0; argv[i] != NULL; ++i) {
      free(argv[i]);
    }
  }
}
