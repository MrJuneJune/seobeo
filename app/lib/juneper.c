#include "juneper.h"

int FindChildrenFromParentGeneric(
  const void* parent, const int parent_len,
  const void* children, const int children_len, 
  size_t type_size
) {
  // need to cast or we can't do pointer arithmetics
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

void GetTimeStamp(char* time_stamp, size_t size) {
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  strftime(time_stamp, size, "%Y-%m-%d %H:%M:%S", tm_info);
}
