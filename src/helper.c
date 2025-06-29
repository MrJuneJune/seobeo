#include <seobeo/helper.h>

void GetTimeStamp(char *time_stamp, size_t size)
{
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  strftime(time_stamp, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

int CreateHashPos(const char *key, const size_t size)
{
  // DJD hash number
  int hash_val = 5381;
  int c;
  while ((c = *key++))
  {
    hash_val = (hash_val << 5) + hash_val + c;
  }
  return hash_val % size;
}

HashMap *CreateHashMap(size_t size, void (*free_value)(void *))
{
  HashMap *hash_map = malloc(sizeof(HashMap));
  hash_map->size = size;
  hash_map->entries = calloc(size, sizeof(Entry *));
  hash_map->free_value = free_value;

  return hash_map;
}

void InsertHashMap(HashMap *hash_map, const char *key, void *value)
{
  int idx = CreateHashPos(key, hash_map->size);
  Entry *entry = hash_map->entries[idx];
  while (entry)
  {
    if (strcmp(entry->key, key) == 0)
    {
      hash_map->free_value(entry->value);
      entry->value = value;
      return;
    }
    entry = entry->next;
  }
  Entry *new_entry = malloc(sizeof(Entry));
  new_entry->key = strdup(key);
  new_entry->value = value;
  new_entry->next = hash_map->entries[idx];
  hash_map->entries[idx] = new_entry;
}

void *GetHashMapValue(HashMap *hash_map, const char *key)
{
  int idx = CreateHashPos(key, hash_map->size);
  Entry *entry = hash_map->entries[idx];
  while (entry)
  {
    printf("key: %s\n", entry->key);
    if (strcmp(entry->key, key) == 0)
    {
      return entry->value;
    }
    entry = entry->next;
  }
  // none matching
  return NULL;
}

void FreeHashMap(HashMap *hash_map)
{
  for(int i = 0; i < hash_map->size; i++)
  {
    Entry *entry = hash_map->entries[i];
    while (entry)
    {
      Entry *next = entry->next;
      free(entry->key);
      hash_map->free_value(entry->value);
      free(entry);
      entry = next;
    }
  }
  free(hash_map->entries);
  free(hash_map);
}

Arena *ArenaCreate(size_t size)
{
  Arena *arena = malloc(sizeof(Arena));
  if (!arena) return NULL;
  arena->buffer = malloc(size);
  if (!arena->buffer) {
    free(arena);
    return NULL;
  }
  arena->capacity = size;
  arena->offset = 0;
  return arena;
}

void *ArenaAlloc(Arena *arena, size_t size)
{
  if (arena->offset + size > arena->capacity) return NULL;
  void *ptr = arena->buffer + arena->offset;
  arena->offset += size;
  return ptr;
}

void ArenaReset(Arena *arena)
{
  arena->offset = 0;
}

void ArenaDestroy(Arena *arena)
{
  if (arena) {
    free(arena->buffer);
    free(arena);
  }
}

