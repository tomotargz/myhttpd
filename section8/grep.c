#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

static void do_grep(regex_t* p, FILE* f);

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s PATTERN [FILE...]\n", argv[0]);
    exit(1);
  }
  regex_t p;
  int err = regcomp(&p, argv[1], REG_EXTENDED | REG_NOSUB | REG_NEWLINE);
  if (err) {
    char b[1024];
    regerror(err, &p, b, sizeof b);
    puts(b);
    exit(1);
  }

  if (argc == 2) {
    do_grep(&p, stdin);
  } else {
    for (int i = 2; i < argc; ++i) {
      FILE* f = fopen(argv[i], "r");
      if (!f) {
        perror(argv[i]);
        exit(1);
      }
      do_grep(&p, f);
      fclose(f);
    }
  }
  regfree(&p);
  exit(0);
}

static void do_grep(regex_t* p, FILE* f) {
  char b[4096];
  while (fgets(b, sizeof b, f)) {
    if (regexec(p, b, 0, NULL, 0) == 0) {
      fputs(b, stdout);
    }
  }
}
