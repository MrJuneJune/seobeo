// --- Custom Libs --- 
#include <seobeo/helper.h>
#include <seobeo/server.h>

volatile sig_atomic_t stop_server = 0;

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

  // Gracefully stop...
  signal(SIGINT, handle_sigint);

  // Using Epoll Fd to assign server_fd and client_fd.
  RunEpollLoop(server_fd);

  // TODO: Maybe assign fd into arena and clean up at the end easily.
  printf("Shutting down server...\n");
  close(server_fd);
  return 0;
}

