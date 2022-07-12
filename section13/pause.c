#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void hello(int s) {
  printf("signal received!\n");
  fflush(stdout);
}

int main(int argc, char* argv[]) {
  printf("wait for receiving a signal...\n");

  struct sigaction act;
  act.sa_handler = hello;
  act.sa_flags = SA_RESTART;
  sigemptyset(&act.sa_mask);

  if (sigaction(SIGINT, &act, NULL) == -1) {
    perror("sigaction");
  }
  pause();
  return 0;
}
