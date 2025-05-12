#ifndef SEOBEO_HELPER_H
#define SEOBEO_HELPER_H

#include <string.h>
#include <stdio.h>
#include <time.h>

// TODO: Change so that requests, and response are properly handled using arena.
typedef struct {
  char* buffer;
  size_t capacity;
  size_t offset;
} Arena;

void GetTimeStamp(char* time_stamp, size_t size);

#endif // SEOBEO_HELPER_H
