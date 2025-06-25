#ifndef SEOBEO_HELPER_H
#define SEOBEO_HELPER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_HASH_LENGTH 1000


typedef struct Entry {
  char *key;
  void *value;
  struct Entry *next;
} Entry;

typedef struct {
  Entry **entries;
  size_t size;
  void (*free_value)(void *);
} HashMap;

typedef struct {
  char *buffer;
  size_t capacity;
  size_t offset;
} Arena;

// For logging..
void GetTimeStamp(char *time_stamp, size_t size);

// HashMaps
int CreateHashPos(const char *key, const size_t size);
HashMap *CreateHashMap(size_t size, void (*free_value)(void *));
void InsertHashMap(HashMap *hash_map, const char *key, void *value);
void *GetHashMapValue(HashMap *hash_map, const char *key);
void FreeHashMap(HashMap *hash_map);

// malloc Arena and buffer. It returns NULL when it is out of memory to create these.
Arena *ArenaCreate(size_t size);
// Allocate size if size + buffer is smaller than capacity else return nullptr.
void *ArenaAlloc(Arena *arena, size_t size);
// Set offset to 0. Does not free.
void ArenaReset(Arena *arena);
// Free Both Areana and its buffer
void ArenaDestroy(Arena *arena);


#endif // SEOBEO_HELPER_H
