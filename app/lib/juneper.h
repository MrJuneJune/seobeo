#ifndef JUNEPER_H
#define JUNEPER_H

#include <string.h>
#include <stdio.h>
#include <time.h>

typedef struct {
  char* buffer;
  size_t capacity;
  size_t offset;
} Arena;

int FindChildrenFromParentGeneric(
  const void* parent, const int parent_len,
  const void* children, const int children_len, 
  size_t type_size
);

void GetTimeStamp(char* time_stamp, size_t size);

#endif // JUNEPER_H
