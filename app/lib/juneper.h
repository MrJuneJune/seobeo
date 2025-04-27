#ifndef JUNEPER_H
#define JUNEPER_H

#include <string.h>
#include <stdio.h>
#include <regex.h>
#include <time.h>

int FindChildrenFromParentGeneric(
  const void* parent, const int parent_len,
  const void* children, const int children_len, 
  size_t type_size
);
void* ExtractPathFromReferer(const char* string_value, char* out_path);
void* GetTimeStamp(char* time_stamp, size_t size);

#endif // JUNEPER_H
