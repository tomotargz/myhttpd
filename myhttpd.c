#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
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

static void log_exit(char* fmt, ...);
static void* xmalloc(size_t s);
static void install_signal_handlers(void);
static void trap_signal(int sig, sighandler_t handler);
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

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s DOC_ROOT\n", argv[0]);
    exit(1);
  }
  install_signal_handlers();
  service(stdin, stdout, argv[1]);
  exit(0);
}

static void log_exit(char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
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
static const char* SERVER_NAME = "myHTTP";
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
