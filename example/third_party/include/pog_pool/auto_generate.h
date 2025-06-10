#ifndef GENERATE_FILES
#define GENERATE_FILES

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define LINE_LEN 1024
#define TABLE_LEN 64 
#define COLUNM_LEN 64 
#define MAX_COLUNM_NUM 64 
#define QUERY_BUFFER 1024
#define DIR_LEN 512
#define FILE_LEN 256
#define DATABASE_URL_LEN 512

// This should be removed when we handle other keys
#define CREATE_TABLE 13
#define IS_CREATE_TABLE(str) (strncmp(str, "CREATE TABLE", 12) == 0)

// This is so we can turn lists back into psotgres version
#define ArrayToString(arr, len, is_json) \
  _Generic((arr), \
    int*: ArrayToString_int, \
    char**: ArrayToString_str \
  )(arr, len, is_json)

typedef struct {
  char name[COLUNM_LEN];
  char type[COLUNM_LEN];
} Column;

typedef struct {
  char input_dir[DIR_LEN];
  char output_dir[DIR_LEN];
  char output_file[FILE_LEN];
  char database_url[DATABASE_URL_LEN];
} PogPoolConfig;

typedef struct {
  char column_names[512];
  char format_parts[512];
  char value_args[1024];
  char set_clause[1024];
  char update_args[1024];
  char field_assignments[4096];
  char serialize_format[2048];
  char serialize_args[2048];
} CodegenOutput;

typedef enum {
  ARRAY_TYPE_INT,
  ARRAY_TYPE_STRING
} ArrayElementType;

// TODO: Split these up?
// Helper functions 
const char *SqlToCType(const char *sql_type); 
char *SkipSpaces(char *p); 
int GetWord(char *line, char *word);
pid_t *PIDStatus(const char *model_dir, const struct dirent *entry, FILE *models_header, const char *output_folder);
char *SanitizeHexForJSON(const char *input);
char *ArrayToString_int(const int *arr, int len, int is_json);
char *ArrayToString_str(char** arr, int len, int is_json);
void *ParseArray(const char *value, ArrayElementType type, size_t *out_len);

// Core SQL to C logic
void CreateCRUDFiles(const char *table_name, const Column *columns, size_t column_sizes, const char *output_folder); 
void CreateCRUDFilesFromSQL(const char *sql_file_name, const char *output_folder);
void GenerateModelFilesFromConfig();

// Configuration related
void EnsureDefaultConfig(const char *config_path);
void ParseConfig(const char *config_path, PogPoolConfig *config);

#endif // GENERATE_FILES

