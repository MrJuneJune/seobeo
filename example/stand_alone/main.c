#include <seobeo/server.h>
#include <seobeo/helper.h>
#include <seobeo/os.h>

#define NUM_WORKERS 4
pthread_t workers[NUM_WORKERS];
ClientQueue client_queue;
HashMap *static_file;

volatile sig_atomic_t stop_server = 0;

void clicked(int client_fd, HttpRequestType *request)
{
  char response_header_buffer[BUFFER_SIZE];
  const char *content_type = "text/html";
  char response[BUFFER_SIZE];

  char *tmp = "<div> JUNE PARK </div>";
  strcpy(response, tmp);

  CreateHTTPResponse(client_fd, response, content_type, response_header_buffer);
  return;
}

Route ROUTE[] = {
  {
    HTTP_METHOD_POST,
    "/clicked",
    &clicked
  },
};
size_t ROUTE_SIZE = 1;

void handle_sigint(int sig)
{
    stop_server = 1;
}

// --- main server loop ---
int main()
{
  InitGlobalVariables();

  int server_fd;
  struct sockaddr_in server_addr;

  // Assign 8mb for caching static files.
  static_file = CreateHashMap(
    1000, 
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
