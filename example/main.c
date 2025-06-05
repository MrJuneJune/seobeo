// --- Custom Libs --- 
#include <seobeo/helper.h>
#include <seobeo/server.h>
#include "../lib/connection.h"

volatile sig_atomic_t stop_server = 0;
volatile ConnectionPool* connection_pool;

void handle_sigint(int sig) {
  stop_server = 1;
}

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
  InitPool(connection_pool, "postgres://mrjunejune:june@localhost:5432/mrjunejune");

  // Gracefully stop...
  signal(SIGINT, handle_sigint);

  // Using Epoll Fd to assign server_fd and client_fd.
  RunEpollLoop(server_fd);

  // TODO: Maybe assign fd into arena and clean up at the end easily.
  printf("Shutting down server...\n");
  close(server_fd);
  return 0;
}

