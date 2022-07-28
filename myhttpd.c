#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <grp.h>
#include <netdb.h>
#include <pwd.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

struct HTTPHeaderField {
  char* name;
  char* value;
  struct HTTPHeaderField* next;
};

struct HTTPRequest {
  int protocol_minor_version;
  char* method;
  char* path;
  struct HTTPHeaderField* header;
  char* body;
  long length;
};

struct FileInfo {
  char* path;
  long size;
  int ok;
};

static void log_exit(const char* fmt, ...);
static void* xmalloc(size_t s);
static void install_signal_handlers(void);
static void trap_signal(int sig, sighandler_t handler);
static void detach_children(void);
static void noop_handler(int sig);
static void signal_exit(int sig);
static void service(FILE* in, FILE* out, char* docroot);
static void free_request(struct HTTPRequest* req);
static struct HTTPRequest* read_request(FILE* in);
static void read_request_line(struct HTTPRequest* req, FILE* in);
static struct HTTPHeaderField* read_header_field(FILE* in);
static long content_length(struct HTTPRequest* req);
static char* lookup_header_field_value(struct HTTPRequest* req, char* name);
static struct FileInfo* get_fileinfo(char* docroot, char* urlpath);
static void free_fileinfo(struct FileInfo* f);
static char* build_fspath(char* docroot, char* urlpath);
static void respond_to(struct HTTPRequest* req, FILE* out, char* docroot);
static void do_file_response(struct HTTPRequest* req, FILE* out, char* docroot);
static void method_not_allowed(struct HTTPRequest* req, FILE* out);
static void not_implemented(struct HTTPRequest* req, FILE* out);
static void not_found(struct HTTPRequest* req, FILE* out);
static void output_common_header_fields(struct HTTPRequest* req, FILE* out,
                                        char* status);
static char* guess_content_type(struct FileInfo* f);
static void upcase(char* str);
static int listen_socket(char* path);
static void server_main(int server_fd, char* doc_root);
static void become_daemon(void);
static void setup_environment(char* root, char* user, char* group);

static const char* USAGE =
    "Usage: %s [--port=n] [--chroot --user=u --group=g] <docroot>\n";

static int debug_mode = 0;
static int do_chroot = 0;
static char* user = NULL;
static char* group = NULL;
static char* port = NULL;
static char* docroot = NULL;

static struct option longopts[] = {
    {"debug", no_argument, &debug_mode, 1},
    {"chroot", no_argument, NULL, 'c'},
    {"user", required_argument, NULL, 'u'},
    {"group", required_argument, NULL, 'g'},
    {"port", required_argument, NULL, 'p'},
    {"help", no_argument, NULL, 'h'},
    {0, 0, 0, 0},
};

static const char* SERVER_NAME = "myHTTP";

int main(int argc, char* argv[]) {
  int opt;
  while ((opt = getopt_long(argc, argv, "", longopts, NULL)) != -1) {
    switch (opt) {
      case 0:
        break;
      case 'c':
        do_chroot = 1;
        break;
      case 'u':
        user = optarg;
        break;
      case 'g':
        group = optarg;
        break;
      case 'p':
        port = optarg;
        break;
      case 'h':
        fprintf(stdout, USAGE, argv[0]);
      case '?':
        fprintf(stderr, USAGE, argv[0]);
        exit(1);
    }
  }
  if (optind != argc - 1) {
    fprintf(stderr, USAGE, argv[0]);
    exit(1);
  }
  docroot = argv[optind];
  if (do_chroot) {
    setup_environment(docroot, user, group);
    docroot = NULL;
  }
  install_signal_handlers();
  int server_fd = listen_socket(port);
  if (!debug_mode) {
    openlog(SERVER_NAME, LOG_PID | LOG_NDELAY, LOG_DAEMON);
    become_daemon();
  }
  server_main(server_fd, docroot);
  exit(0);
}

static void log_exit(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  if (debug_mode) {
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
  } else {
    vsyslog(LOG_ERR, fmt, ap);
  }
  va_end(ap);
  exit(1);
}

static void* xmalloc(size_t s) {
  void* p = malloc(s);
  if (!p) log_exit("failed to allocate memory");
  return p;
}

static void install_signal_handlers(void) {
  trap_signal(SIGPIPE, signal_exit);
  detach_children();
}

static void trap_signal(int sig, sighandler_t handler) {
  struct sigaction act;
  act.sa_handler = handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART;
  if (sigaction(sig, &act, NULL) < 0) {
    log_exit("sigaction() failed: %s", strerror(errno));
  }
}

static void detach_children(void) {
  struct sigaction act;
  act.sa_handler = noop_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = SA_RESTART | SA_NOCLDWAIT;
  if (sigaction(SIGCHLD, &act, NULL) < 0) {
    log_exit("sigaction() failed: %s", strerror(errno));
  }
}

static void noop_handler(int sig) {
  ;
}

static void signal_exit(int sig) {
  log_exit("exit by signal %d", sig);
}

static void service(FILE* in, FILE* out, char* docroot) {
  struct HTTPRequest* req = read_request(in);
  respond_to(req, out, docroot);
  free_request(req);
}

static void free_request(struct HTTPRequest* req) {
  for (struct HTTPHeaderField* h = req->header; h;) {
    struct HTTPHeaderField* next = h->next;
    free(h->name);
    free(h->value);
    free(h);
    h = next;
  }
  free(req->method);
  free(req->path);
  free(req->body);
}

static const int MAX_REQUEST_BODY_LENGTH = 1024;

static struct HTTPRequest* read_request(FILE* in) {
  struct HTTPRequest* req = xmalloc(sizeof(struct HTTPRequest));
  read_request_line(req, in);
  req->header = NULL;
  struct HTTPHeaderField* h;
  while ((h = read_header_field(in))) {
    h->next = req->header;
    req->header = h;
  }
  req->length = content_length(req);
  if (req->length == 0) {
    req->body = NULL;
    return req;
  }

  if (req->length > MAX_REQUEST_BODY_LENGTH) {
    log_exit("request body too long");
  }
  req->body = xmalloc(req->length);
  if (fread(req->body, req->length, 1, in) < 1) {
    log_exit("failed to read request body");
  }
  return req;
}

static const size_t LINE_BUF_SIZE = 1024;

static void read_request_line(struct HTTPRequest* req, FILE* in) {
  char buf[LINE_BUF_SIZE];
  if (!fgets(buf, sizeof buf, in)) {
    log_exit("no request line");
  }
  char* p = strchr(buf, ' ');
  if (!p) log_exit("parse error on request line (1): %s", buf);
  *p++ = '\0';
  req->method = xmalloc(p - buf);
  strcpy(req->method, buf);
  upcase(req->method);
  char* path = p;
  p = strchr(path, ' ');
  if (!p) log_exit("parse error on request line (2): %s", buf);
  *p++ = '\0';
  req->path = xmalloc(p - path);
  strcpy(req->path, path);
  if (strncasecmp(p, "HTTP/1.", strlen("HTTP/1."))) {
    log_exit("parse error on request line (3): %s", buf);
  }
  p += strlen("HTTP/1.");
  req->protocol_minor_version = atoi(p);
}

static void upcase(char* str) {
  char* c = str;
  while (*c) {
    if ('a' <= *c && *c <= 'z') {
      *c += 'A' - 'a';
    }
    ++c;
  }
}

static struct HTTPHeaderField* read_header_field(FILE* in) {
  char buf[LINE_BUF_SIZE];
  if (!fgets(buf, LINE_BUF_SIZE, in)) {
    log_exit("failed to read request header field: %s", strerror(errno));
  }
  if (buf[0] == '\n' || !strcmp(buf, "\r\n")) return NULL;
  char* p = strchr(buf, ':');
  if (!p) log_exit("parse error on request header field: %s", buf);
  *p++ = '\0';
  struct HTTPHeaderField* h = xmalloc(sizeof(struct HTTPHeaderField));
  h->name = xmalloc(p - buf);
  strcpy(h->name, buf);

  p += strspn(p, " \t");
  h->value = xmalloc(strlen(p) + 1);
  strcpy(h->value, p);
  return h;
}

static long content_length(struct HTTPRequest* req) {
  char* val = lookup_header_field_value(req, "Content-Length");
  if (!val) return 0;
  long len = atol(val);
  if (len < 0) log_exit("negative Content-length value");
  return len;
}

static char* lookup_header_field_value(struct HTTPRequest* req, char* name) {
  for (struct HTTPHeaderField* h = req->header; h; h = h->next) {
    if (!strcasecmp(h->name, name)) return h->value;
  }
  return NULL;
}

static struct FileInfo* get_fileinfo(char* docroot, char* urlpath) {
  struct FileInfo* info = xmalloc(sizeof(struct FileInfo));
  info->path = build_fspath(docroot, urlpath);
  info->ok = 0;
  struct stat st;
  if (lstat(info->path, &st) < 0) return info;
  if (!S_ISREG(st.st_mode)) return info;
  info->ok = 1;
  info->size = st.st_size;
  return info;
}

static void free_fileinfo(struct FileInfo* f) {
  free(f->path);
  free(f);
}

static char* build_fspath(char* docroot, char* urlpath) {
  char* path = xmalloc(strlen(docroot) + strlen(urlpath) + 2);
  sprintf(path, "%s/%s", docroot, urlpath);
  return path;
}

static void respond_to(struct HTTPRequest* req, FILE* out, char* docroot) {
  if (!strcmp(req->method, "GET")) {
    do_file_response(req, out, docroot);
  } else if (!strcmp(req->method, "HEAD")) {
    do_file_response(req, out, docroot);
  } else if (!strcmp(req->method, "POST")) {
    method_not_allowed(req, out);
  } else {
    not_implemented(req, out);
  }
}

static const size_t BLOCK_BUF_SIZE = 4096;

static void do_file_response(struct HTTPRequest* req, FILE* out,
                             char* docroot) {
  struct FileInfo* info = get_fileinfo(docroot, req->path);
  if (!info->ok) {
    free_fileinfo(info);
    not_found(req, out);
    return;
  }
  output_common_header_fields(req, out, "200 OK");
  fprintf(out, "Content-Length: %ld\r\n", info->size);
  fprintf(out, "Content-Type: %s\r\n", guess_content_type(info));
  fprintf(out, "\r\n");
  if (strcmp(req->method, "HEAD")) {
    int fd = open(info->path, O_RDONLY);
    if (fd < 0) log_exit("failed to open %s: %s", info->path, strerror(errno));
    while (1) {
      char buf[BLOCK_BUF_SIZE];
      ssize_t n = read(fd, buf, BLOCK_BUF_SIZE);
      if (n < 0) log_exit("failed to read %s: %s", info->path, strerror(errno));
      if (n == 0) break;
      if (fwrite(buf, n, 1, out) < 1) {
        log_exit("failed to write to socket: %s", strerror(errno));
      }
    }
    close(fd);
  }
  fflush(out);
  free_fileinfo(info);
}

static void method_not_allowed(struct HTTPRequest* req, FILE* out) {
  output_common_header_fields(req, out, "405 Method Not Allowed");
}

static void not_implemented(struct HTTPRequest* req, FILE* out) {
  output_common_header_fields(req, out, "400 Bad Request");
}

static void not_found(struct HTTPRequest* req, FILE* out) {
  output_common_header_fields(req, out, "404 Not Found");
}

static const size_t TIME_BUF_SIZE = 1024;
static const int HTTP_MINOR_VERSION = 0;
static const char* SERVER_VERSION = "1.0";

static void output_common_header_fields(struct HTTPRequest* req, FILE* out,
                                        char* status) {
  time_t t = time(NULL);
  struct tm* tm = gmtime(&t);
  if (!tm) log_exit("gmtime() failed: %s", strerror(errno));
  char buf[TIME_BUF_SIZE];
  strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S GMT", tm);
  fprintf(out, "HTTP/1.%d %s\r\n", HTTP_MINOR_VERSION, status);
  fprintf(out, "Date: %s\r\n", buf);
  fprintf(out, "Server: %s/%s\r\n", SERVER_NAME, SERVER_VERSION);
  fprintf(out, "Connection: close\r\n");
}

static char* guess_content_type(struct FileInfo* f) {
  return "text/plain";
}

static const int MAX_BACKLOG = 5;

static int listen_socket(char* path) {
  struct addrinfo hints;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  struct addrinfo* res;
  int err = getaddrinfo(NULL, port, &hints, &res);
  if (err) log_exit(gai_strerror(err));
  for (struct addrinfo* ai = res; ai; ai = ai->ai_next) {
    int sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (sock < 0) continue;
    if (bind(sock, ai->ai_addr, ai->ai_addrlen) < 0) {
      close(sock);
      continue;
    }
    if (listen(sock, MAX_BACKLOG) < 0) {
      close(sock);
      continue;
    }
    freeaddrinfo(res);
    return sock;
  }
  log_exit("failed to listen socket");
  return -1;
}

static void server_main(int server_fd, char* doc_root) {
  for (;;) {
    struct sockaddr_storage addr;
    socklen_t addrlen = sizeof addr;
    int sock = accept(server_fd, (struct sockaddr*)&addr, &addrlen);
    if (sock < 0) log_exit("accept(2) failed: %s", strerror(errno));
    int pid = fork();
    if (pid < 0) exit(3);
    if (pid == 0) {
      FILE* in = fdopen(sock, "r");
      FILE* out = fdopen(sock, "w");
      service(in, out, docroot);
      exit(0);
    }
    close(sock);
  }
}

static void become_daemon(void) {
  if (chdir("/") < 0) log_exit("chdir(2) failed: %s", strerror(errno));
  freopen("/dev/null", "r", stdin);
  freopen("/dev/null", "w", stdout);
  freopen("/dev/null", "w", stderr);
  int pid = fork();
  if (pid < 0) log_exit("fork(2) failed: %s", strerror(errno));
  if (pid != 0) _exit(0);
  if (setsid() < 0) log_exit("setsid(2) failed: %s", strerror(errno));
}

static void setup_environment(char* root, char* user, char* group) {
  if (!user) {
    fprintf(stderr, "user not specified\n");
    exit(1);
  }
  if (!group) {
    fprintf(stderr, "group not specidied\n");
    exit(1);
  }
  struct group* g = getgrnam(group);
  if (!g) {
    fprintf(stderr, "no such group: %s\n", group);
  }
  if (setgid(g->gr_gid) < 0) {
    perror("setgid(2)");
    exit(1);
  }
  if (initgroups(user, g->gr_gid) < 0) {
    perror("initgroups(2)");
    exit(1);
  }
  struct passwd* pw = getpwnam(user);
  if (!pw) {
    fprintf(stderr, "no such user: %s\n", user);
    exit(1);
  }
  chroot(root);
  if (setuid(pw->pw_uid) < 0) {
    perror("setuid(2)");
    exit(1);
  }
}
