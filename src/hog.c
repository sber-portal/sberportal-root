#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char** argv) {
  long mb = (argc > 1) ? atol(argv[1]) : 800;
  size_t sz = (size_t)mb * 1024UL * 1024UL;
  char* p = malloc(sz);
  if (!p) { fprintf(stderr, "malloc %ldmb failed\n", mb); return 1; }
  for (size_t i = 0; i < sz; i += 4096) p[i] = 1;
  fprintf(stderr, "hog: held %ldmb, sleeping\n", mb);
  sleep(600);
  return 0;
}
