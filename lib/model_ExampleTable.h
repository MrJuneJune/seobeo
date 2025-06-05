#ifndef MODEL_ExampleTable
#define MODEL_ExampleTable

#include <postgresql/libpq-fe.h>

typedef struct {
  char* id;
  short small_int_col;
  int int_col;
  long long big_int_col;
  double decimal_col;
  double numeric_col;
  void* real_col;
  double double_col;
  void* serial_col;
  char* char_col;
  char* varchar_col;
  char* text_col;
  char* date_col;
  char* time_col;
  char* timestamp_col;
  char* timestamptz_col;
  int boolean_col;
  char* another_uuid;
  void* json_col;
  void* jsonb_col;
  int int_array_col;
  char* text_array_col;
  void* status_col;
  void* file_col;
} ExampleTable;

typedef struct {
  ExampleTable* ExampleTable;
  ExecStatusType status;
} ExampleTableQuery;

ExampleTableQuery QueryExampleTable(PGconn* conn, const char* where_clause);
void InsertExampleTable(PGconn* conn, ExampleTable u);
void UpdateExampleTable(PGconn* conn, ExampleTable u, const char* where_clause);
void DeleteExampleTable(PGconn* conn, const char* where_clause);

#endif // MODEL_ExampleTable
