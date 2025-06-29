#include <seobeo/os.h>
#include <seobeo/server.h>
#include <sys/epoll.h>

#ifdef LINUX 

static int global_initial_fd = -1;

void CleanupClient(int client_fd)
{
  // Disable Nagle's algorithm (optional for closed socket, but harmless)
  int opt = 1;
  setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

  if (global_initial_fd != -1)
  {
    // epoll
    epoll_ctl(global_initial_fd, EPOLL_CTL_DEL, client_fd, NULL);
  }

  close(client_fd);
}

void RunEventLoop(const int server_fd)
{
  struct epoll_event events_buffer[MAX_EVENTS];
  // EPOLLIN means ready for reading
  struct epoll_event ev = {
    .events = EPOLLIN,
    .data.fd = server_fd
  };
  int epfd = epoll_create1(0);
  if (epfd == -1)
  {
    perror("epoll_create1");
    return;
  }
  
  // Set global epfd for cleanup
  global_initial_fd = epfd;

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev) == -1)
  {
    perror("epoll_ctl: server_fd");
    close(epfd);
    return;
  }
  
  while (!stop_server)
  {
    // Find open events and overwrite to events 
    int n = epoll_wait(epfd, events_buffer, MAX_EVENTS, 1000); // 1 second timeout
    
    if (n == -1)
    {
      if (errno == EINTR) continue; // Interrupted by signal
      perror("epoll_wait");
      break;
    }
  
    // Look through all events that are waiting
    for (int i = 0; i < n; i++)
    {
      int fd = events_buffer[i].data.fd;
  
      // If it is the server socket, it means a new client is trying to connect.
      if (fd == server_fd)
      {
        while (1)
        {
          // Create a new client socket where client can bind to socket_fd
          int client_fd = accept(server_fd, NULL, NULL);
          // No more connections to accept to given server
          if (client_fd == -1)
          {
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
              perror("accept");
            }
            break;
          }

          if (SetNonBlocking(client_fd) == -1)
          {
            perror("SetNonBlocking");
            close(client_fd);
            continue;
          }
          
          // Set TCP_NODELAY for new client connections
          int opt = 1;
          setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
  
          // Register this client socket with epoll.
          // using EPOLLET so that it notifies when the fd is ready?
          // not really sure about this one, but apparently more performant
          struct epoll_event client_ev = {
            .events = EPOLLIN | EPOLLET,  
            .data.fd = client_fd
          };
          if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_ev) == -1)
          {
            perror("epoll_ctl: client_fd");
            close(client_fd);
            continue;
          }
        }
      }
      else
      {
        Enqueue(&client_queue, fd);
      }
    }
  }

  close(epfd);
  global_initial_fd = -1;
}

#endif // LINUX
