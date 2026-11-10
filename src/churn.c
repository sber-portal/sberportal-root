#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

/* Page-cache churn: repeatedly read a file into a buffer.
 * Keeps the page cache hot without consuming anonymous memory, so the
 * Mali shrinker fires but MemAvailable stays healthy (no OOM/panic).
 * usage: churn <file> [mb]
 */
int main(int argc, char** argv) {
  const char* path = argc > 1 ? argv[1] : "/vendor/priv-app/StarCvService/StarCvService.apk";
  long mb = argc > 2 ? atol(argv[2]) : 64;
  size_t sz = (size_t)mb * 1024UL * 1024UL;
  char* buf = malloc(sz);
  if (!buf) { fprintf(stderr, "malloc %ldmb failed\n", mb); return 1; }
  memset(buf, 0, sz);
  fprintf(stderr, "churn: %s buf=%ldmb\n", path, mb);
  for (;;) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) { usleep(200000); continue; }
    ssize_t r;
    while ((r = read(fd, buf, sz)) > 0) { buf[r - 1] = buf[0]; }
    close(fd);
  }
  return 0;
}
