#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>       
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>   
#include <arpa/inet.h>   
#include <sys/epoll.h>

// --- Custom Libs --- 
#include "lib/juneper.h"

#define PORT 6969 
#define BUFFER_SIZE 8192
#define MAX_EVENTS 64

#define RESPONSE_HEADER "HTTP/1.1 200 OK\r\n"\
  "Content-Type: text/html\r\n"\
  "Connection: close\r\n"\
  "\r\n"

#define GET_HEADER "GET "
#define REFERER_HEADER "Referer: "

int SetNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void CreateSocket(int* server_fd) {
  *server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (*server_fd == -1) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }
  SetNonBlocking(*server_fd);
}

void BindToSocket(int* server_fd, struct sockaddr_in* server_addr) {
  server_addr->sin_family = AF_INET;
  server_addr->sin_addr.s_addr = INADDR_ANY;
  server_addr->sin_port = htons(PORT);

  if (bind(*server_fd, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
    perror("bind failed");
    close(*server_fd);
    exit(EXIT_FAILURE);
  }
}

void ListenToSocket(int* server_fd) {
  if (listen(*server_fd, SOMAXCONN) < 0) {
    perror("listen failed");
    close(*server_fd);
    exit(EXIT_FAILURE);
  }
  printf("🚀 HTTP Server listening on port %d...\n", PORT);
}

void HandleRequest(int client_fd) {
  char header_buffer[BUFFER_SIZE] = {0};
  char response_buffer[BUFFER_SIZE] = {0};
  char path[1024] = {0};
  char file_path[1024] = {0};

  ssize_t bytes_read = read(client_fd, header_buffer, BUFFER_SIZE - 1);
  if (bytes_read <= 0) {
    close(client_fd);
    return;
  }

  int start_pos = FindChildrenFromParentGeneric(
    header_buffer, BUFFER_SIZE,
    GET_HEADER, strlen(GET_HEADER),
    sizeof(char)
  );

  if (start_pos != -1) {
    ExtractPathFromReferer(header_buffer + start_pos, path);
  }

  if (strcmp(path, "/") == 0) {
    snprintf(file_path, sizeof(file_path), "paths/index.html");
  } else if (strstr(path, ".svg") || strstr(path, ".ico") || strstr(path, ".png")) {
    snprintf(file_path, sizeof(file_path), "assets%s", path);
  } else {
    snprintf(file_path, sizeof(file_path), "paths%s.html", path);
  }

  FILE *html_file = fopen(file_path, "r");
  if (!html_file) {
    printf("⚠️ Could not open %s\n", file_path);
    close(client_fd);
    return;
  }

  send(client_fd, RESPONSE_HEADER, strlen(RESPONSE_HEADER), 0);

  while (fgets(response_buffer, BUFFER_SIZE, html_file)) {
    send(client_fd, response_buffer, strlen(response_buffer), 0);
  }

  fclose(html_file);
  close(client_fd);
}

int main() {
  int server_fd;
  struct sockaddr_in server_addr;

  CreateSocket(&server_fd);
  BindToSocket(&server_fd, &server_addr);
  ListenToSocket(&server_fd);

  int epfd = epoll_create1(0);
  struct epoll_event ev = {
    .events = EPOLLIN,
    .data.fd = server_fd
  };
  epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

  struct epoll_event events[MAX_EVENTS];

  while (1) {
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
    for (int i = 0; i < n; i++) {
      int fd = events[i].data.fd;

      if (fd == server_fd) {
        // Accept all pending connections
        while (1) {
          int client_fd = accept(server_fd, NULL, NULL);
          if (client_fd == -1) break;

          SetNonBlocking(client_fd);

          struct epoll_event client_ev = {
            .events = EPOLLIN | EPOLLET,
            .data.fd = client_fd
          };
          epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_ev);
        }
      } else {
        HandleRequest(fd);
      }
    }
  }

  close(server_fd);
  return 0;
}

