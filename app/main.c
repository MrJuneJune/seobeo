#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>       
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>   
#include <arpa/inet.h>   
#include <sys/epoll.h>
#include <signal.h>

// --- Custom Libs --- 
#include "lib/juneper.h"
#include "lib/junerver.h"

volatile sig_atomic_t stop_server = 0;

void handle_sigint(int sig) {
  stop_server = 1;
}

int main() {
  int server_fd;
  struct sockaddr_in server_addr;

  // starting server
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

