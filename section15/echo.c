#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

static int open_connection(const char* host, const char* service);

int main(int argc, char* argv[]) {
  char wb[1024];
  while (fgets(wb, sizeof wb, stdin)) {
    int sock = open_connection((argc > 1 ? argv[1] : "localhost"), "echo");
    FILE* echod = fdopen(sock, "w+");
    if (!echod) {
      perror("fdopen(3)");
      exit(1);
    }
    fprintf(echod, "%s\n", wb);
    fflush(echod);
    char rb[1024];
    fgets(rb, sizeof rb, echod);
    printf("echo: %s", rb);
    fclose(echod);
  }
}

static int open_connection(const char* host, const char* service) {
  struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, 0, 0, 0, 0, 0};
  struct addrinfo* res;

  int err = getaddrinfo(host, service, &hints, &res);
  if (err) {
    fprintf(stderr, "getaddrinfo(3): %s\n", gai_strerror(err));
    exit(1);
  }
  for (struct addrinfo* ai = res; ai; ai = ai->ai_next) {
    int sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sock < 0) continue;
    if (connect(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
      close(sock);
      continue;
    }
    freeaddrinfo(res);
    return sock;
  }
  fprintf(stderr, "socket(2)/connect(2) failed");
  freeaddrinfo(res);
  exit(1);
}
