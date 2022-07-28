#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: %s COMMAND ARG\n", argv[0]);
    exit(1);
  }

  pid_t pid = fork();
  if (pid < 0) {
    fprintf(stderr, "fork(2) failed\n");
    exit(1);
  } else if (pid == 0) {
    execlp(argv[1], argv[1], argv[2]);
    perror(argv[1]);
    exit(99);
  } else {
    int status;
    waitpid(pid, &status, 0);
    printf("child (PID=%d) finished: ", pid);
    if (WIFEXITED(status)) {
      printf("exit, status=%d\n", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
      printf("signal, sig=%d\n", WTERMSIG(status));
    } else {
      printf("abnormal exit\n");
    }
    exit(0);
  }
}
