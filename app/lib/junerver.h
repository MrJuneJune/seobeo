#ifndef JUNERVER_H
#define JUNERVER_H

#include <sys/socket.h>
#include <unistd.h>       
#include <fcntl.h>
#include <netinet/in.h>   
#include <sys/socket.h>
#include <sys/epoll.h>
#include <arpa/inet.h>   

#define PORT 6969  // good number
#define BUFFER_SIZE 8192 // ngnix default I believe
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

// Related to response
#define RESPONSE_HEADER "HTTP/1.1 200 OK\r\n"\
  "Content-Type: text/html; charset=utf-8\r\n"\
  "Connection: close\r\n"\
  "\r\n"
#define GET_HEADER "GET "
#define REFERER_HEADER "Referer: "

int SetNonBlocking(int fd);
void CreateSocket(int* server_fd);
void BindToSocket(int* server_fd, struct sockaddr_in* server_addr);
void ListenToSocket(int* server_fd);
int SetupEpoll(int server_fd);
void HandleRequest(int client_fd);
void GenerateResponseHeader(char* buffer, int status, const char* content_type);
void SendHTTPErrorResponse(int client_fd, int status_code);

#endif // JUNERVER_H
