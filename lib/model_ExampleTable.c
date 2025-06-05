#include "model_ExampleTable.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

ExampleTableQuery QueryExampleTable(PGconn* conn, const char* where_clause)
{
  ExampleTableQuery query_result;
  char query[1024];
  snprintf(query, sizeof(query), "SELECT * FROM ExampleTable WHERE %s;", where_clause);
  PGresult* res = PQexec(conn, query);
  ExecStatusType status = PQresultStatus(res);  query_result.ExampleTable = NULL;
  query_result.status = status;  if (status != PGRES_TUPLES_OK)
  {
    fprintf(stderr, "SELECT failed: %s\n", PQerrorMessage(conn));
    PQclear(res);
    return query_result;
  }
  int rows = PQntuples(res);
   if (rows == 0)
   {
     return query_result;
   }
  ExampleTable* list = malloc(rows * sizeof(ExampleTable));
  for (int i = 0; i < rows; ++i)
  {
    list[i].id = strdup(PQgetvalue(res, i, 0));
    list[i].small_int_col = atoi(PQgetvalue(res, i, 1));
    list[i].int_col = atoi(PQgetvalue(res, i, 2));
    list[i].big_int_col = atoi(PQgetvalue(res, i, 3));
    list[i].decimal_col = atof(PQgetvalue(res, i, 4));
    list[i].numeric_col = atof(PQgetvalue(res, i, 5));
    list[i].real_col = strdup(PQgetvalue(res, i, 6));
    list[i].double_col = atof(PQgetvalue(res, i, 7));
    list[i].serial_col = strdup(PQgetvalue(res, i, 8));
    list[i].char_col = strdup(PQgetvalue(res, i, 9));
    list[i].varchar_col = strdup(PQgetvalue(res, i, 10));
    list[i].text_col = strdup(PQgetvalue(res, i, 11));
    list[i].date_col = strdup(PQgetvalue(res, i, 12));
    list[i].time_col = strdup(PQgetvalue(res, i, 13));
    list[i].timestamp_col = strdup(PQgetvalue(res, i, 14));
    list[i].timestamptz_col = strdup(PQgetvalue(res, i, 15));
    list[i].boolean_col = atoi(PQgetvalue(res, i, 16));
    list[i].another_uuid = strdup(PQgetvalue(res, i, 17));
    list[i].json_col = strdup(PQgetvalue(res, i, 18));
    list[i].jsonb_col = strdup(PQgetvalue(res, i, 19));
    list[i].int_array_col = atoi(PQgetvalue(res, i, 20));
    list[i].text_array_col = strdup(PQgetvalue(res, i, 21));
    list[i].status_col = strdup(PQgetvalue(res, i, 22));
    list[i].file_col = strdup(PQgetvalue(res, i, 23));
  }
  PQclear(res);
  query_result.ExampleTable = list;  return query_result;}

void InsertExampleTable(PGconn* conn, ExampleTable u)
{
  char query[1024];
  snprintf(query, sizeof(query),
    "INSERT INTO ExampleTable (id, small_int_col, int_col, big_int_col, decimal_col, numeric_col, real_col, double_col, serial_col, char_col, varchar_col, text_col, date_col, time_col, timestamp_col, timestamptz_col, boolean_col, another_uuid, json_col, jsonb_col, int_array_col, text_array_col, status_col, file_col) "
    "VALUES ('%s', %d, %d, %d, %f, %f, '%s', %f, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, '%s', '%s', '%s', %d, '%s', '%s', '%s');",
    u.id, u.small_int_col, u.int_col, u.big_int_col, u.decimal_col, u.numeric_col, u.real_col, u.double_col, u.serial_col, u.char_col, u.varchar_col, u.text_col, u.date_col, u.time_col, u.timestamp_col, u.timestamptz_col, u.boolean_col, u.another_uuid, u.json_col, u.jsonb_col, u.int_array_col, u.text_array_col, u.status_col, u.file_col);
  PGresult* res = PQexec(conn, query);
  PQclear(res);
}

void UpdateExampleTable(PGconn* conn, ExampleTable u, const char* where_clause)
{
  char query[1024];
  snprintf(query, sizeof(query),
    "UPDATE ExampleTable "
    "SET id='%s', small_int_col=%d, int_col=%d, big_int_col=%d, decimal_col=%f, numeric_col=%f, real_col='%s', double_col=%f, serial_col='%s', char_col='%s', varchar_col='%s', text_col='%s', date_col='%s', time_col='%s', timestamp_col='%s', timestamptz_col='%s', boolean_col=%d, another_uuid='%s', json_col='%s', jsonb_col='%s', int_array_col=%d, text_array_col='%s', status_col='%s', file_col='%s' WHERE %s;",
    u.id, u.small_int_col, u.int_col, u.big_int_col, u.decimal_col, u.numeric_col, u.real_col, u.double_col, u.serial_col, u.char_col, u.varchar_col, u.text_col, u.date_col, u.time_col, u.timestamp_col, u.timestamptz_col, u.boolean_col, u.another_uuid, u.json_col, u.jsonb_col, u.int_array_col, u.text_array_col, u.status_col, u.file_col, where_clause);
  PGresult* res = PQexec(conn, query);
  PQclear(res);
}

void DeleteExampleTable(PGconn* conn, const char* where_clause)
{
  char query[1024];
  snprintf(query, sizeof(query),
    "DELETE FROM ExampleTable WHERE %s;",
    where_clause);
  PGresult* res = PQexec(conn, query);
  PQclear(res);
}
