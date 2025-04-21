#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>       
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>   
#include <arpa/inet.h>   
#include <sys/epoll.h>

#include "junerver.h"
#include "juneper.h"


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
