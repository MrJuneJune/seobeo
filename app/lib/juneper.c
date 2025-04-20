#include <stdio.h>
#include <string.h>
#include <regex.h>

#include "juneper.h"

int FindChildrenFromParentGeneric(
  const void* parent, const int parent_len,
  const void* children, const int children_len, 
  size_t type_size
) {
  // need to case or we can't do pointer arithmetics
  const char* parent_cast = (const char*)parent;
  const char* children_cast = (const char*)children;

  int res = -1;

  if (children_len == 0 || parent_len < children_len) return res;

  for (int i = 0; i <= parent_len - children_len; i++) {
    if (
      memcmp(parent_cast + i * type_size, children_cast, children_len * type_size) == 0
    )
    {
      return i;
    } 
  }

  return res;
}

void* ExtractPathFromReferer(const char* string_value, char* out_path) {
  regex_t regex;
  regmatch_t matches[2];
  int WHOLE_MATCH_INDEX = 0;
  int PATH_INDEX = 1;

  // TODO: move to consts file
  // const char* pattern = "https?://[^/]+(/[^ \r\n]*)";
  const char* pattern = "(/[^ \r\n]*)";

  if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
    fprintf(stderr, "Failed to compile regex\n");
    return out_path;
  }

  if (regexec(&regex, string_value, 2, matches, 0) == 0) {
    int start = matches[1].rm_so;
    int end = matches[1].rm_eo;

    strncpy(out_path, string_value + start, end - start);
    printf("📂 Extracted path: %s\n", out_path);
  } else {
    printf("⚠️  Referer path not found.\n");
  }

  regfree(&regex);

  return out_path;
}
