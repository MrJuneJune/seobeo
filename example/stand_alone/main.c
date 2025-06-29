#include <seobeo/server.h>
#include <seobeo/helper.h>
#include <seobeo/os.h>

Route ROUTE[] = {0};
size_t ROUTE_SIZE = 1;
HashMap *static_file;

volatile sig_atomic_t stop_server = 0;

void handle_sigint(int sig)
{
    stop_server = 1;
}

// --- main server loop ---
int main()
{
  int server_fd;
  struct sockaddr_in server_addr;

  // Assign 8mb for caching static files.
  static_file = CreateHashMap(
    8388608, // 8 MB
    FreeStaticFileEntry     
  );

  // Starting server
  CreateSocket(&server_fd);
  BindToSocket(&server_fd, &server_addr);
  ListenToSocket(&server_fd);

  // Gracefully stop...
  signal(SIGINT, handle_sigint);

  // Using Epoll Fd to assign server_fd and client_fd.
  RunEventLoop(server_fd);

  // TODO: Maybe assign fd into arena and clean up at the end easily.
  printf("Shutting down server...\n");
  close(server_fd);
  return 0;
}
