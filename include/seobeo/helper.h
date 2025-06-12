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

// Arena
Arena *ArenaCreate(size_t size);
void *ArenaAlloc(Arena *arena, size_t size);
void ArenaReset(Arena *arena);
void ArenaDestroy(Arena *arena);


#endif // SEOBEO_HELPER_H
