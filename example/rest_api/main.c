// --- Custom Libs --- 
#include <seobeo/helper.h>
#include <seobeo/server.h>
#include <seobeo/os.h>
#include "pog_pool/connection.h"
#include "pog_pool/auto_generate.h"
#include "build/models.h"
#include <jansson.h>

volatile sig_atomic_t stop_server = 0;
volatile ConnectionPool *connection_pool;
HashMap *static_file;

void handle_sigint(int sig)
{
  stop_server = 1;
}

// --- Routing logic ---
void handleGetExampleTable(int client_fd, HttpRequestType *request)
{
  char response_header_buffer[BUFFER_SIZE];
  const char *content_type = "application/json";
  char response[BUFFER_SIZE];

  char where_clause[1024];
  for (size_t i = 0; i < request->path_params_len; i++)
  {
    if (strcmp(request->path_params[i].key, "id") == 0)
    {
      const char *id = request->path_params[i].value;
      sprintf(where_clause, "id='%s'", id);
      WriteToLogs("Requested ID = %s", id);
      break;
    }
  }

  PGconn *pg_conn = BorrowConnection(connection_pool);
  ExampleTableQuery etq = QueryExampleTable(pg_conn, "*",where_clause);
  if (etq.ExampleTable != NULL)
  {
    char *json = SerializeExampleTable(*etq.ExampleTable);
    sprintf(response, json);
  }
  else
  {
    char *tmp = "{\"status\": \"failed\"}\0";
    strcpy(response, tmp);
  }

  // clean up
  ReleaseConnection(connection_pool, pg_conn) ;
  FreeExampleTableQuery(&etq);

  CreateHTTPResponse(client_fd, response, content_type, response_header_buffer);
  return;
}

void handleGetExampleTableHTML(int client_fd, HttpRequestType *request)
{
  char response_header_buffer[BUFFER_SIZE];
  const char *content_type = "text/html; charset=utf-8";
  char response[BUFFER_SIZE];
  char file_path[BUFFER_SIZE];
  char *file_buffers = malloc(sizeof(char)*100000);

  snprintf(file_path, sizeof(file_path), "paths/index.seobeo");
  FILE *file = fopen(file_path, "rb");
  if (!file)
  {
    WriteToLogs("[Error]⚠️ Could not open %s\n", file_path);
    SendHTTPErrorResponse(client_fd, HTTP_NOT_FOUND);
    return;
  }

  struct stat st;
  if (stat(file_path, &st) != 0)
  {
    WriteToLogs("[Error]⚠️ Could not stat %s\n", file_path);
    SendHTTPErrorResponse(client_fd, HTTP_NOT_FOUND);
    return;
  }
  size_t file_size = st.st_size;
  fread(file_buffers, file_size, 1, file);

  char where_clause[1024];
  for (size_t i = 0; i < request->path_params_len; i++)
  {
    if (strcmp(request->path_params[i].key, "id") == 0)
    {
      const char *id = request->path_params[i].value;
      sprintf(where_clause, "id='%s'", id);
      WriteToLogs("Requested ID = %s", id);
      break;
    }
  }

  PGconn *pg_conn = BorrowConnection(connection_pool);
  ExampleTableQuery etq = QueryExampleTable(pg_conn, "*",where_clause);

  if (etq.ExampleTable != NULL)
  {
    ReplaceAll(&file_buffers, "{{TEXT_VALUE}}", etq.ExampleTable->text_col);
  }
  else 
  {
    // TODO: Support generic failure?
  }


  // clean up
  ReleaseConnection(connection_pool, pg_conn) ;
  FreeExampleTableQuery(&etq);

  CreateHTTPResponse(client_fd, file_buffers, content_type, response_header_buffer);
  // Free file buffer since it is no longer needed.... or maybe we do this later and store this?
  free(file_buffers);
  return;
}

void handlePostFoo(int client_fd, HttpRequestType *request)
{
  char response_header_buffer[BUFFER_SIZE];
  const char *content_type = "application/json";
  char *response;

  printf("Request: %s\n", request->body);

  json_error_t error;
  json_t *root = json_loads(request->body, 0, &error);
  if (!root)
  {
    response = "{\"error\": \"Invalid JSON\"}";
    goto response;
  }

  ExampleTable example = {
    .id = strdup(json_string_value(json_object_get(root, "id"))),
    .small_int_col = json_integer_value(json_object_get(root, "small_int_col")),
    .int_col = json_integer_value(json_object_get(root, "int_col")),
    .big_int_col = json_integer_value(json_object_get(root, "big_int_col")),
    .decimal_col = json_real_value(json_object_get(root, "decimal_col")),
    .numeric_col = json_real_value(json_object_get(root, "numeric_col")),
    .real_col = json_real_value(json_object_get(root, "real_col")),
    .double_col = json_real_value(json_object_get(root, "double_col")),
    .serial_col = NULL,
    .char_col = strdup(json_string_value(json_object_get(root, "char_col"))),
    .varchar_col = strdup(json_string_value(json_object_get(root, "varchar_col"))),
    .text_col = strdup(json_string_value(json_object_get(root, "text_col"))),
    .date_col = strdup(json_string_value(json_object_get(root, "date_col"))),
    .time_col = strdup(json_string_value(json_object_get(root, "time_col"))),
    .timestamp_col = strdup(json_string_value(json_object_get(root, "timestamp_col"))),
    .timestamptz_col = strdup(json_string_value(json_object_get(root, "timestamptz_col"))),
    .boolean_col = json_boolean_value(json_object_get(root, "boolean_col")),
    .another_uuid = strdup(json_string_value(json_object_get(root, "another_uuid"))),
    .json_col = strdup(json_dumps(json_object_get(root, "json_col"), 0)),
    .jsonb_col = strdup(json_dumps(json_object_get(root, "jsonb_col"), 0)),
    .int_array_col = NULL,
    .int_array_col_len = 0,
    .text_array_col = NULL,
    .text_array_col_len = 0,
    .status_col = strdup(json_string_value(json_object_get(root, "status_col"))),
    .file_col = strdup(json_string_value(json_object_get(root, "file_col")))
  };

  // --- int_array_col ---
  json_t *int_arr = json_object_get(root, "int_array_col");
  if (json_is_array(int_arr))
  {
    size_t len = json_array_size(int_arr);
    example.int_array_col_len = len;
    example.int_array_col = malloc(sizeof(int) * len);
    for (size_t i = 0; i < len; ++i)
    {
      json_t *val = json_array_get(int_arr, i);
      example.int_array_col[i] = json_integer_value(val);
    }
  }
  else
  {
    example.int_array_col = NULL;
    example.int_array_col_len = 0;
  }

  // --- text_array_col ---
  json_t *text_arr = json_object_get(root, "text_array_col");
  if (json_is_array(text_arr))
  {
    size_t len = json_array_size(text_arr);
    example.text_array_col_len = len;
    example.text_array_col = malloc(sizeof(char *) * len);
    for (size_t i = 0; i < len; ++i)
    {
      json_t *val = json_array_get(text_arr, i);
      const char *str = json_string_value(val);
      example.text_array_col[i] = strdup(str ? str : "");
    }
  }
  else
  {
    example.text_array_col = NULL;
    example.text_array_col_len = 0;
  }

  PGconn *pg_conn = BorrowConnection(connection_pool);
  ExampleTableQuery etq = InsertExampleTable(pg_conn, example);
  if (etq.status == PGRES_FATAL_ERROR)
  {
    response = "{\"insert\": \"failed\"}";
  }
  else
  {
    response = "{\"insert\": \"successful\"}";
  }
  ReleaseConnection(connection_pool, pg_conn);
  FreeExampleTableQuery(&etq);

  json_decref(root); // Free memory
  goto response;

response:
  CreateHTTPResponse(client_fd, response, content_type, response_header_buffer);
 }

void handlePutFoo(int client_fd, HttpRequestType *request)
{
  char response_header_buffer[BUFFER_SIZE];
  const char *content_type = "application/json";
  char *response;

  json_error_t error;
  json_t *root = json_loads(request->body, 0, &error);
  if (!root)
  {
    response = "{\"error\": \"Invalid JSON\"}";
    goto response;
  }

  ExampleTable example = {
    .id = strdup(json_string_value(json_object_get(root, "id"))),
    .small_int_col = json_integer_value(json_object_get(root, "small_int_col")),
    .int_col = json_integer_value(json_object_get(root, "int_col")),
    .big_int_col = json_integer_value(json_object_get(root, "big_int_col")),
    .decimal_col = json_real_value(json_object_get(root, "decimal_col")),
    .numeric_col = json_real_value(json_object_get(root, "numeric_col")),
    .real_col = json_real_value(json_object_get(root, "real_col")),
    .double_col = json_real_value(json_object_get(root, "double_col")),
    .serial_col = NULL,
    .char_col = strdup(json_string_value(json_object_get(root, "char_col"))),
    .varchar_col = strdup(json_string_value(json_object_get(root, "varchar_col"))),
    .text_col = strdup(json_string_value(json_object_get(root, "text_col"))),
    .date_col = strdup(json_string_value(json_object_get(root, "date_col"))),
    .time_col = strdup(json_string_value(json_object_get(root, "time_col"))),
    .timestamp_col = strdup(json_string_value(json_object_get(root, "timestamp_col"))),
    .timestamptz_col = strdup(json_string_value(json_object_get(root, "timestamptz_col"))),
    .boolean_col = json_boolean_value(json_object_get(root, "boolean_col")),
    .another_uuid = strdup(json_string_value(json_object_get(root, "another_uuid"))),
    .json_col = strdup(json_dumps(json_object_get(root, "json_col"), 0)),
    .jsonb_col = strdup(json_dumps(json_object_get(root, "jsonb_col"), 0)),
    .status_col = strdup(json_string_value(json_object_get(root, "status_col"))),
    .file_col = strdup(json_string_value(json_object_get(root, "file_col")))
  };

  // --- int_array_col ---
  json_t *int_arr = json_object_get(root, "int_array_col");
  if (json_is_array(int_arr))
  {
    size_t len = json_array_size(int_arr);
    example.int_array_col_len = len;
    example.int_array_col = malloc(sizeof(int) * len);
    for (size_t i = 0; i < len; ++i)
    {
      json_t *val = json_array_get(int_arr, i);
      example.int_array_col[i] = json_integer_value(val);
    }
  }
  else
  {
    example.int_array_col = NULL;
    example.int_array_col_len = 0;
  }

  // --- text_array_col ---
  json_t *text_arr = json_object_get(root, "text_array_col");
  if (json_is_array(text_arr))
  {
    size_t len = json_array_size(text_arr);
    example.text_array_col_len = len;
    example.text_array_col = malloc(sizeof(char *) * len);
    for (size_t i = 0; i < len; ++i)
    {
      json_t *val = json_array_get(text_arr, i);
      const char *str = json_string_value(val);
      example.text_array_col[i] = strdup(str ? str : "");
    }
  }
  else
  {
    example.text_array_col = NULL;
    example.text_array_col_len = 0;
  }

  PGconn *pg_conn = BorrowConnection(connection_pool);

  const char *where_clause = "id = 'b6d4c431-f327-4a4a-9345-320aa3cd7e31'";
  ExampleTableQuery etq = UpdateExampleTable(pg_conn, example, where_clause);
  if (etq.status == PGRES_FATAL_ERROR)
  {
    response = "{\"update\": \"failed\"}";
  }
  else
  {
    response = "{\"update\": \"successful\"}";
  }

  ReleaseConnection(connection_pool, pg_conn);
  FreeExampleTableQuery(&etq);
  json_decref(root);
  goto response;

response:
  CreateHTTPResponse(client_fd, response, content_type, response_header_buffer);
}

void handleDeleteFoo(int client_fd, HttpRequestType *request)
{
  char response_header_buffer[BUFFER_SIZE];
  const char *content_type = "application/json";
  char *response;

  PGconn *pg_conn = BorrowConnection(connection_pool);
  char where_clause[1024];
  for (size_t i = 0; i < request->path_params_len; i++)
  {
    if (strcmp(request->path_params[i].key, "id") == 0)
    {
      const char *id = request->path_params[i].value;
      sprintf(where_clause, "id='%s'", id);
      WriteToLogs("Requested ID = %s", id);
      break;
    }
  }
  ExampleTableQuery etq = DeleteExampleTable(pg_conn, where_clause);
  if (etq.status==PGRES_FATAL_ERROR)
  {
    response = "{\"status\": \"failed\"}";
  }
  else
  {
    response = "{\"status\": \"successful\"}";
  }

  ReleaseConnection(connection_pool, pg_conn);
  FreeExampleTableQuery(&etq);
  goto response;

response:
  CreateHTTPResponse(client_fd, response, content_type, response_header_buffer);
}

Route ROUTE[] = {
  {
    HTTP_METHOD_GET,
    "/api/foo/{id}",
    &handleGetExampleTable
  },
  {
    HTTP_METHOD_GET,
    "/foo/{id}",
    &handleGetExampleTableHTML
  },
  {
    HTTP_METHOD_POST,
    "/api/foo",
    &handlePostFoo
  },
  {
    HTTP_METHOD_DELETE,
    "/api/foo/{id}",
    &handleDeleteFoo
  },
  {
    HTTP_METHOD_PUT,
    "/api/foo",
    &handlePutFoo
  }
};
size_t ROUTE_SIZE = 5;

char* ReadSQLFile(char* file_name)
{
  FILE *fp = fopen(file_name, "r");
  if (!fp) 
  {
    perror("Cannot open SQL file");
    return NULL;
  }

  fseek(fp, 0, SEEK_END);
  long length = ftell(fp);
  rewind(fp);

  char* sql = malloc(length+10);

  fread(sql, 1, length, fp);
  sql[length] = '\0'; 
  fclose(fp);

  return sql;
}

// --- main server loop ---
int main()
{
  // Assign 8mb for caching static files.
  static_file = CreateHashMap(
    8388608,
    FreeStaticFileEntry     
  );

  int server_fd;
  struct sockaddr_in server_addr;

  // DB manager
  ConnectionPool connection_pool_real={0};
  connection_pool = &connection_pool_real;
  InitPool(connection_pool, "postgres://pog_pool:pog_pool@localhost:4269/pog_pool");

  // Create table
  PGconn *pg_conn = BorrowConnection(connection_pool);
  char *sql = ReadSQLFile("models/ExampleTable.sql");
  PGresult *res = PQexec(pg_conn, sql);
  PQclear(res);
  ReleaseConnection(connection_pool, pg_conn);
  printf("Created a ExampleTable.\n");

  // Starting server
  CreateSocket(&server_fd);
  BindToSocket(&server_fd, &server_addr);
  ListenToSocket(&server_fd);

  // Gracefully stop...
  signal(SIGINT, handle_sigint);

  // Using Epoll Fd to assign server_fd and client_fd.
  RunEventLoop(server_fd);

  // TODO: Maybe assign fd into arena and clean up at the end easily.
  printf("Shutting down server...\n");
  close(server_fd);
  return 0;
}
