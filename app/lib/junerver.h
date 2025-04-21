#ifndef JUNERVER_H
#define JUNERVER_H

#include <sys/socket.h>

#define PORT 6969  // good number
#define BUFFER_SIZE 8192 // ngnix default I believe
#define MAX_EVENTS 64

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

#endif // JUNERVER_H
