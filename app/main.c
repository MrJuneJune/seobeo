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
  signal(SIGINT, handle_sigint);

  // -- Epoll Logic --
  // Writing it down since I don't really get it.

  // Step 1: Create an epoll instance. This returns an epoll file descriptor.
  int epfd = epoll_create1(0);
  
  // Step 2: Set up the first event to track: the server listening socket.
  // We want epoll to notify us when it becomes readable
  // (i.e., when a new client is trying to connect).
  struct epoll_event ev = {
    .events = EPOLLIN,       // EPOLLIN means "ready for reading"
    .data.fd = server_fd     // Attach this event to the server socket
  };
  
  // Step 3: Register the server socket with the epoll instance.
  // This tells epoll to watch for incoming connection attempts on server_fd.
  epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);
  
  // Step 4: Preallocate space for epoll to write ready events into.
  // When epoll_wait is called, it will populate this array with all triggered events.
  struct epoll_event events[MAX_EVENTS];
  
  // Step 5: Main event loop — run forever
  while (!stop_server) {
    // Step 6: Block until one or more registered file descriptors become ready.
    // epoll_wait fills `events` with up to MAX_EVENTS ready file descriptors.
    // Returns the number of ready fds (`n`).
    int n = epoll_wait(epfd, events, MAX_EVENTS, -1);
  
    // Step 7: Iterate over each ready file descriptor.
    for (int i = 0; i < n; i++) {
      int fd = events[i].data.fd;
  
      // Step 8: If the ready fd is the server socket, it means a new client is trying to connect.
      if (fd == server_fd) {
        // Step 9: Accept all incoming connections in a loop.
        // With EPOLLET (edge-triggered mode), we must accept ALL connections now.
        while (1) {
          int client_fd = accept(server_fd, NULL, NULL);
          if (client_fd == -1) break;  // No more connections to accept
  
          // Step 10: Set the new client socket to non-blocking mode.
          // This is required when using epoll in edge-triggered mode.
          SetNonBlocking(client_fd);
  
          // Step 11: Register this client socket with epoll.
          // Now epoll will notify us when the client sends data.
          struct epoll_event client_ev = {
            .events = EPOLLIN | EPOLLET,  // Notify when readable, only once until re-triggered
            .data.fd = client_fd
          };
          epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_ev);
        }
      } else {
        // Step 12: If the fd is not the server socket, it's a client socket.
        // Handle the request (read the data, respond, and close).
        HandleRequest(fd);
      }
    }
  }

  printf("Shutting down server...\n");
  close(server_fd);
  close(epfd);
  return 0;
}

