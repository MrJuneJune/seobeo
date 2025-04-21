#include <unistd.h>       
#include <sys/socket.h>
#include <netinet/in.h>   
#include <arpa/inet.h>   

#include "junerver.h"

#define PORT 6969 

void CreateSocket(
  int* server_fd
) {
  *server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (*server_fd == -1) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }
}

void BindToSocket(
  int* server_fd,
  struct sockaddr_in* server_addr
) {
  server_addr->sin_family = AF_INET; // IP 4.0
  server_addr->sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
  server_addr->sin_port = htons(PORT); // Can be any port
  if (bind(*server_fd, (const struct sockaddr *)server_addr, sizeof(*server_addr)) < 0) {
    perror("bind failed");
    close(*server_fd);
    exit(EXIT_FAILURE);
  }
}

void ListenToSocket(
  int* server_fd
) {
  if (listen(*server_fd, 1) < 0) {
    perror("listen failed");
    close(*server_fd);
    exit(EXIT_FAILURE);
  }
  printf("HTTP Server listening on port %d...\n", PORT);
}
