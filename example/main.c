// --- Custom Libs --- 
#include <seobeo/helper.h>
#include <seobeo/server.h>
#include "pog_pool/connection.h"
#include "build/models.h"

volatile sig_atomic_t stop_server = 0;
volatile ConnectionPool* connection_pool;

void handle_sigint(int sig) {
  stop_server = 1;
}

// --- Routing logic ---
void handleGetExampleTable(int client_fd, HttpRequestType* request)
{
  char response_header_buffer[BUFFER_SIZE];
  const char* content_type = "application/json";
  char response[BUFFER_SIZE];

  PGconn *pg_conn = BorrowConnection(connection_pool);
  ExampleTableQuery etq = QueryExampleTable(pg_conn, "*","id=\'b6d4c431-f327-4a4a-9345-320aa3cd7e31\'");
  if (etq.ExampleTable != NULL)
  {
    sprintf(response, SerializeExampleTable(*etq.ExampleTable));
  }
  else
  {
    char *tmp = "{\"status\": \"failed\"}\0";
    strcpy(response, tmp);
  }
  ReleaseConnection(connection_pool, pg_conn) ;

  GenerateResponseHeader(response_header_buffer, HTTP_OK, content_type, strlen(response));
  send(client_fd, response_header_buffer, strlen(response_header_buffer), 0);
  send(client_fd,  response, strlen(response), 0);
  return;
}

void handlePostFoo(int client_fd, HttpRequestType* request)
{
  char response_header_buffer[BUFFER_SIZE];
  const char *content_type = "application/json";
  char *response;

  PGconn *pg_conn = BorrowConnection(connection_pool);

  ExampleTable example = {
    .id = "b6d4c431-f327-4a4a-9345-320aa3cd7e31",
    .small_int_col = 1,
    .int_col = 42,
    .big_int_col = 9000000000LL,
    .decimal_col = 12.34,
    .numeric_col = 56.789,
    .real_col = 1.23,
    .double_col = 9.87,
    .serial_col = NULL,
  
    .char_col = "char_data",
    .varchar_col = "varchar_data",
    .text_col = "This is a long text",
  
    .date_col = "2025-06-05",
    .time_col = "12:34:56",
    .timestamp_col = "2025-06-05 12:34:56",
    .timestamptz_col = "2025-06-05 12:34:56+00",
  
    .boolean_col = 1,
    .another_uuid = "d1b355c0-f348-4bcf-b3df-ef95b3a8a3ad",
  
    .json_col = "{\"key\": \"value\"}",
    .jsonb_col = "{\"key\": \"value\"}",
  
    .int_array_col = (int[]){1,2,3},
    .int_array_col_len = 3,
    .text_array_col = (char* []){"apple","banana"},
    .text_array_col_len = 2,
  
    .status_col = "active",
    .file_col = "\\x68656c6c6f" 
  };

  if (InsertExampleTable(pg_conn, example).status == PGRES_FATAL_ERROR)
  {
    response = "{\"insert\": \"failed\"}";
  }
  else
  {
    response = "{\"insert\": \"successful\"}";
  }

  ReleaseConnection(connection_pool, pg_conn);

  GenerateResponseHeader(response_header_buffer, HTTP_OK, content_type, strlen(response));
  send(client_fd, response_header_buffer, strlen(response_header_buffer), 0);
  send(client_fd,  response, strlen(response), 0);
}

void handlePutFoo(int client_fd, HttpRequestType* request)
{
  char response_header_buffer[BUFFER_SIZE];
  const char *content_type = "application/json";
  char *response;

  PGconn *pg_conn = BorrowConnection(connection_pool);

  ExampleTable example = {
    .id = "b6d4c431-f327-4a4a-9345-320aa3cd7e31",
    .small_int_col = 1,
    .int_col = 42,
    .big_int_col = 9000000000LL,
    .decimal_col = 12.34,
    .numeric_col = 56.789,
    .real_col = 1.23,
    .double_col = 9.87,
    .serial_col = NULL,
  
    .char_col = "updated",
    .varchar_col = "varchar_data",
    .text_col = "This is a long text",
  
    .date_col = "2025-06-05",
    .time_col = "12:34:56",
    .timestamp_col = "2025-06-05 12:34:56",
    .timestamptz_col = "2025-06-05 12:34:56+00",
  
    .boolean_col = 1,
    .another_uuid = "d1b355c0-f348-4bcf-b3df-ef95b3a8a3ad",
  
    .json_col = "{\"key\": \"value\"}",
    .jsonb_col = "{\"key\": \"value\"}",
  
    .int_array_col = (int[]){1,2,3},
    .int_array_col_len = 3,
    .text_array_col = (char* []){"apple","banana"},
    .text_array_col_len = 2,
  
    .status_col = "active",
    .file_col = "\\x68656c6c6f" 
  };

  if (UpdateExampleTable(pg_conn, example, "id = 'b6d4c431-f327-4a4a-9345-320aa3cd7e31'").status == PGRES_FATAL_ERROR)
  {
    response = "{\"insert\": \"failed\"}";
  }
  else
  {
    response = "{\"insert\": \"successful\"}";
  }

  ReleaseConnection(connection_pool, pg_conn);

  GenerateResponseHeader(response_header_buffer, HTTP_OK, content_type, strlen(response));
  send(client_fd, response_header_buffer, strlen(response_header_buffer), 0);
  send(client_fd,  response, strlen(response), 0);
}

void handleDeleteFoo(int client_fd, HttpRequestType* request)
{
  char response_header_buffer[BUFFER_SIZE];
  const char *content_type = "application/json";
  char *response;

  PGconn *pg_conn = BorrowConnection(connection_pool);

  char *where_clause = "id = \'b6d4c431-f327-4a4a-9345-320aa3cd7e31\'";
  if (DeleteExampleTable(pg_conn, where_clause).status==PGRES_FATAL_ERROR)
  {
    response = "{\"status\": \"failed\"}";
  }
  else
  {
    response = "{\"status\": \"successful\"}";
  }

  ReleaseConnection(connection_pool, pg_conn);

  GenerateResponseHeader(response_header_buffer, HTTP_OK, content_type, strlen(response));
  send(client_fd, response_header_buffer, strlen(response_header_buffer), 0);
  send(client_fd,  response, strlen(response), 0);
}

PathToHandler GET_REQUEST_HANDLER[] = {
  {
    "/api/foo",
    &handleGetExampleTable
  }
};
size_t GET_REQUEST_HANDLER_SIZE =  1;


PathToHandler POST_REQUEST_HANDLER[] = {
  {
    "/api/foo",
    &handlePostFoo
  }
};
size_t POST_REQUEST_HANDLER_SIZE =  1;


PathToHandler DELETE_REQUEST_HANDLER[] = {
  {
    "/api/foo",
    &handleDeleteFoo
  }
};

size_t DELETE_REQUEST_HANDLER_SIZE =  1;

PathToHandler PUT_REQUEST_HANDLER[] = {
  {
    "/api/foo",
    &handlePutFoo
  }
};
size_t PUT_REQUEST_HANDLER_SIZE =  1;

// --- main server loop ---
int main() {
  int server_fd;
  struct sockaddr_in server_addr;

  // Starting server
  CreateSocket(&server_fd);
  BindToSocket(&server_fd, &server_addr);
  ListenToSocket(&server_fd);

  // DB manager
  ConnectionPool connection_pool_real={0};
  connection_pool = &connection_pool_real;
  InitPool(connection_pool, "postgres://pog_pool:pog_pool@localhost:4269/pog_pool");

  // Gracefully stop...
  signal(SIGINT, handle_sigint);

  // Using Epoll Fd to assign server_fd and client_fd.
  RunEpollLoop(server_fd);

  // TODO: Maybe assign fd into arena and clean up at the end easily.
  printf("Shutting down server...\n");
  close(server_fd);
  return 0;
}

