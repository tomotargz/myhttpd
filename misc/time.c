#include <time.h>
#include <stdio.h>
int main(int argc, char* argv[]){
  time_t now = time(NULL);
  struct tm* local_time = localtime(&now);
  struct tm* utc_time = gmtime(&now);
  printf("local: %s", asctime(local_time));
  printf("UTC  : %s", asctime(utc_time));
  char buf[1024];
  strftime(buf, sizeof buf, "%FT%T%z", local_time);
  printf("local: %s\n", buf);
  strftime(buf, sizeof buf, "%FT%T%z", utc_time);
  printf("UTC  : %s\n", buf);
}
