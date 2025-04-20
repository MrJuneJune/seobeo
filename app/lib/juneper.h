#ifndef JUNEPER_H
#define JUNEPER_H

#include <stddef.h>

int FindChildrenFromParentGeneric(
  const void* parent, const int parent_len,
  const void* children, const int children_len, 
  size_t type_size
);

void* ExtractPathFromReferer(const char* string_value, char* out_path) ;

#endif // JUNEPER_H
