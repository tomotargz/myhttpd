#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

static void do_slice(regex_t* p, FILE* f);

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s PATTERN [FILE...]\n", argv[0]);
  }
  regex_t p;
  int err = regcomp(&p, argv[1], REG_EXTENDED | REG_NEWLINE);
  if (err) {
    char buf[1024];
    regerror(err, &p, buf, sizeof buf);
    fputs(buf, stderr);
    exit(1);
  }

  if (argc == 2) {
    do_slice(&p, stdin);
  } else {
    for (int i = 2; i < argc; ++i) {
      FILE* f = fopen(argv[i], "r");
      if (!f) {
        perror(argv[i]);
        exit(1);
      }
      do_slice(&p, f);
      fclose(f);
    }
  }
  regfree(&p);
}

void do_slice(regex_t* p, FILE* f) {
  char buf[4096];
  regmatch_t m[10];
  while (fgets(buf, sizeof buf, f)) {
    if (regexec(p, buf, 10, m, 0) != 0) continue;
    for (int i = 0; i < 10; ++i) {
      if (m[i].rm_so == -1) break;
      for (int j = m[i].rm_so; j < m[i].rm_eo; ++j) {
        fputc((int)buf[j], stdout);
      }
      fputc('\n', stdout);
    }
  }
}
