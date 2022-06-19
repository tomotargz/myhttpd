#include <getopt.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

static void do_grep(regex_t* p, FILE* f, int invert_match);

int main(int argc, char* argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s [-iv] PATTERN [FILE...]\n", argv[0]);
    exit(1);
  }

  int ignore_case = 0;
  int invert_match = 0;
  int opt;
  while ((opt = getopt(argc, argv, "iv")) != -1) {
    switch (opt) {
      case 'i':
        ignore_case = 1;
        break;
      case 'v':
        invert_match = 1;
        break;
      case '?':
        fprintf(stderr, "Usage: %s [-iv] PATTERN [FILE...]\n", argv[0]);
        exit(1);
    }
  }

  regex_t p;
  int regcomp_flags = REG_EXTENDED | REG_NOSUB | REG_NEWLINE;
  if (ignore_case) regcomp_flags |= REG_ICASE;
  int err = regcomp(&p, argv[optind], regcomp_flags);
  if (err) {
    char b[1024];
    regerror(err, &p, b, sizeof b);
    puts(b);
    exit(1);
  }
  ++optind;

  if (optind == argc) {
    do_grep(&p, stdin, invert_match);
  } else {
    for (int i = optind; i < argc; ++i) {
      FILE* f = fopen(argv[i], "r");
      if (!f) {
        perror(argv[i]);
        exit(1);
      }
      do_grep(&p, f, invert_match);
      fclose(f);
    }
  }
  regfree(&p);
  exit(0);
}

static void do_grep(regex_t* p, FILE* f, int invert_match) {
  char b[4096];
  while (fgets(b, sizeof b, f)) {
    int match = (regexec(p, b, 0, NULL, 0) == 0);
    if ((match && !invert_match) || (!match && invert_match)) {
      fputs(b, stdout);
    }
  }
}
