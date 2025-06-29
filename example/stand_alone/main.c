#include <seobeo/server.h>
#include <seobeo/helper.h>
#include <seobeo/os.h>

#define NUM_WORKERS 4
pthread_t workers[NUM_WORKERS];
ClientQueue client_queue;

Route ROUTE[] = {0};
size_t ROUTE_SIZE = 1;
HashMap *static_file;
ClientJob *current_client_job;
ClientJob *root_client_job;

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

  current_client_job = malloc(sizeof(ClientJob));
  root_client_job = current_client_job;

  // Assign 8mb for caching static files.
  static_file = CreateHashMap(
    8388608 * 2, // 8 MB
    FreeStaticFileEntry     
  );

  // Starting server
  CreateSocket(&server_fd);
  BindToSocket(&server_fd, &server_addr);
  ListenToSocket(&server_fd);

  // Gracefully stop...
  signal(SIGINT, handle_sigint);

  // GPT: Multi thread
  InitQueue(&client_queue);
  for (int i = 0; i < NUM_WORKERS; ++i)
  {
      pthread_create(&workers[i], NULL, WorkerThread, &client_queue);
  }

  // Using Epoll Fd to assign server_fd and client_fd.
  RunEventLoop(server_fd);

  // TODO: Maybe assign fd into arena and clean up at the end easily.
  printf("Shutting down server...\n");
  close(server_fd);
  return 0;
}
