#ifndef GENERATE_FILES
#define GENERATE_FILES


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define LINE_LEN 1024
#define TABLE_LEN 64 
#define COLUNM_LEN 64 
#define MAX_COLUNM_NUM 64 
#define QUERY_BUFFER 1024

// This should be removed when we handle other keys
#define CREATE_TABLE 13
#define IS_CREATE_TABLE(str) (strncmp(str, "CREATE TABLE", 12) == 0)


typedef struct {
  char name[COLUNM_LEN];
  char type[COLUNM_LEN];
} Column;

const char* SqlToCType(const char* sql_type); 

char* SkipSpaces(char* p); 

int GetWord(char* line, char* word);

void CreateCRUDFiles(const char* table_name, const Column* columns, size_t column_sizes); 

void CreateCRUDFilesFromSQL(const char *sql_file_name);

#endif // GENERATE_FILES

