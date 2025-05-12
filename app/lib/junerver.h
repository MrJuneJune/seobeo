#ifndef JUNERVER_H
#define JUNERVER_H

#include <arpa/inet.h>   
#include <fcntl.h>
#include <netinet/in.h>   
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <regex.h>
#include <unistd.h>       
#include <sys/epoll.h>
#include <errno.h>
#include <signal.h>

#define PORT 6969  // good number
#define BUFFER_SIZE 8192 // ngnix default I believe
#define LOGGER_BUFFER 8192
#define MAX_EVENTS 64

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

#define GET_HEADER "GET "
#define POST_HEADER "POST "
#define PUT_HEADER "PUT "
#define DELETE_HEADER "DELETE "
#define CONTENT_LENGTH_HEADER "Content-Length: "
#define CONTENT_TYPE_HEADER "Content-Type: "

// TODO: Create a global.h file for stuff like this.
extern volatile sig_atomic_t stop_server;

typedef struct {
  int method;
  char path[1024];
  char* body;
  int content_length; 
  char content_type[128];
} HttpRequestType;

typedef void RequestHandler(int client_fd, HttpRequestType* request);

typedef struct {
  const char* path;
  RequestHandler* handler;
} PathToHandler;

// Server Related
int SetNonBlocking(int fd);
void CreateSocket(int* server_fd);
void BindToSocket(int* server_fd, struct sockaddr_in* server_addr);
void ListenToSocket(int* server_fd);

// Request Related
void ParseHttpRequest(char* buffer, HttpRequestType* request);
void ExtractPathFromReferer(const char* string_value, char* out_path); 
int  SanitizePaths(char* path);
void HandleGetRequest(int client_fd, HttpRequestType* request);
void HandlePostRequest(int client_fd, HttpRequestType* request);
void HandlePutRequest(int client_fd, HttpRequestType* request);
void HandleDeleteRequest(int client_fd, HttpRequestType* request);
void HandleRequest(int client_fd);

// Response Related
void GenerateResponseHeader(char* buffer, int status, const char* content_type, const int content_length);
void SendHTTPErrorResponse(int client_fd, int status_code);

// Loggers
void WriteRequestLog(HttpRequestType request);
void WriteToLogs(const char *restrict format, ...);

// Epoll Logci
void RunEpollLoop(const int server_fd);

#endif // JUNERVER_H
