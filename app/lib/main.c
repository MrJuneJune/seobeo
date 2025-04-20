#include "juneper.h"

int main() {
  char* res;
  char path[1024] = {0};

  ExtractPathFromReferer("June Referer: https://localhost:6969/you/dead", path);

  return 0;
}
