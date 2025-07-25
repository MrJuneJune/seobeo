#pragma once
#ifndef SEOBEO_SERVER_H
#define SEOBEO_SERVER_H

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <regex.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include <seobeo/helper.h>

// third party
// #include <jansson.h>

#define PORT 6969  // good number
#define BUFFER_SIZE 8192  // ngnix default I believe
#define STATIC_FILE_BUFFER 1048576
#define LOGGER_BUFFER 8192
#define MAX_EVENTS 1000
#define MAX_QUERY_LEN 1024
#define MAX_PATH_LEN 1024
#define MAX_CONTENT_TYPE_LEN 128

#define MAX_PATH_PARAMS 4
#define MAX_KEY_LEN 64
#define MAX_VALUE_LEN 256
#define REQUEST_ARENA_SIZE 32768 // 32kb

// HTTP STATUS CODE
#define HTTP_OK 200
#define HTTP_CREATED 201
#define HTTP_MOVED_PERMANENTLY 301
#define HTTP_FOUND 302
#define HTTP_BAD_REQUEST 400
#define HTTP_UNAUTHORIZED 401
#define HTTP_FORBIDDEN 403
#define HTTP_NOT_FOUND 404
#define HTTP_INTERNAL_ERROR 500

// Maybe make this into enum?
#define HTTP_METHOD_UNDEFINED 0
#define HTTP_METHOD_GET 1
#define HTTP_METHOD_POST 2
#define HTTP_METHOD_PUT 3
#define HTTP_METHOD_DELETE 4

// HTTP HEADER
#define GET_HEADER "GET "
#define POST_HEADER "POST "
#define PUT_HEADER "PUT "
#define DELETE_HEADER "DELETE "
// TODO: FIX THIS.
#define CONTENT_LENGTH_HEADER "Content-Length: "
#define CONTENT_TYPE_HEADER "Content-Type: "

// TODO: Create a global.h file for stuff like this.
extern volatile sig_atomic_t stop_server;
extern HashMap *static_file;

typedef struct {
  char key[MAX_KEY_LEN];
  char value[MAX_VALUE_LEN];
} PathParam;

typedef struct {
  int method;
  HashMap *headers;
  char path[MAX_PATH_LEN];
  char *body;
  int content_length; 
  char content_type[MAX_CONTENT_TYPE_LEN];
  char query[MAX_QUERY_LEN];

  PathParam path_params[MAX_PATH_PARAMS];
  size_t path_params_len;
} HttpRequestType;

typedef void RequestHandler(int client_fd, HttpRequestType *request);

typedef struct {
  int method; 
  const char *path_pattern;
  RequestHandler *handler;
} Route;

typedef struct {
  char *data;
  size_t size;
  const char *content_type;
} StaticFileEntry;

// Create a separate router header and src file to handle these.
extern Route ROUTE[];
extern size_t ROUTE_SIZE;

// Server Related
int SetNonBlocking(int fd);
void CreateSocket(int *server_fd);
void BindToSocket(int *server_fd, struct sockaddr_in *server_addr);
void ListenToSocket(int *server_fd);
int SendAll(int sockfd, const void *buf, size_t len);

// Request Related
void ParseHttpRequest(char *buffer, HttpRequestType *request, Arena *request_arena);
void ExtractPathFromReferer(const char *string_value, char *out_path, char *out_query); 
int  SanitizePaths(char *path);
void HandleRoutes(int client_fd, HttpRequestType *request, Route *routes, size_t route_count);
void HandleRequest(int client_fd);

// Response Related
void GenerateResponseHeader(char *buffer, int status, const char *content_type, const int content_length);
void SendHTTPErrorResponse(int client_fd, int status_code);
void CreateHTTPResponse(int client_fd, char *response, const char *content_type, char *response_header_buffer);
StaticFileEntry *LoadStaticFile(const char *file_path, const char *content_type);
void FreeStaticFileEntry(void *entry_ptr);

// Loggers
void WriteRequestLog(HttpRequestType request);
void WriteToLogs(const char *restrict format, ...);

#endif // SEOBEO_SERVER_H
