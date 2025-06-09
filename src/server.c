#include <seobeo/helper.h>
#include <seobeo/server.h>

// --- Response -- //
void GenerateResponseHeader
(
  char* buffer,
  int status,
  const char* content_type,
  const int content_length
) {
  const char* status_text;
  

  // TODO: Make this into compile time variables 
  switch(status) {
    case HTTP_OK: status_text = "OK"; break;
    case HTTP_CREATED: status_text = "Created"; break;
    case HTTP_MOVED_PERMANENTLY: status_text = "Moved Permanently"; break;
    case HTTP_FOUND: status_text = "Found"; break;
    case HTTP_BAD_REQUEST: status_text = "Bad Request"; break;
    case HTTP_UNAUTHORIZED: status_text = "Unauthorized"; break;
    case HTTP_FORBIDDEN: status_text = "Forbidden"; break;
    case HTTP_NOT_FOUND: status_text = "Not Found"; break;
    case HTTP_INTERNAL_ERROR: status_text = "Internal Server Error"; break;
    default: status_text = "Unknown"; break;
  }
  
  sprintf(
    buffer, 
    "HTTP/1.1 %d %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n"
    "\r\n", 
    status, status_text, content_type, content_length
  );
}

void SendHTTPErrorResponse(int client_fd, int status_code)
{
  char header[BUFFER_SIZE] = {0};
  char body[BUFFER_SIZE] = {0};
  
  // TODO: Create a 404 page or user this. 
  sprintf(body, 
      "<html><head><title>Error %d</title></head>"
      "<body><h1>Error %d</h1><p>%s</p></body></html>",
      status_code, status_code, 
      status_code == HTTP_NOT_FOUND ? "The requested resource was not found." :
      status_code == HTTP_FORBIDDEN ? "You don't have permission to access this resource." :
      "An error occurred processing your request.");
  
  GenerateResponseHeader(header, status_code, "text/html", strlen(body));
  
  send(client_fd, header, strlen(header), 0);
  send(client_fd, body, strlen(body), 0);
}

// --- Server -- //
int SetNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) return -1;
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

  // Re-use tcp address?
  int opt = 1;
  setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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
  WriteToLogs("🚀 HTTP Server listening on port %d...\n", PORT);
}

void ExtractPathFromReferer(const char* string_value, char* out_path) {
  regex_t regex;
  regmatch_t matches[2];
  int WHOLE_MATCH_INDEX = 0;
  int PATH_INDEX = 1;

  // TODO: move to consts file
  // const char* pattern = "https?://[^/]+(/[^ \r\n]*)";
  const char* pattern = "(/[^ \r\n]*)";

  // Add loggers
  if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
    fprintf(stderr, "Failed to compile regex\n");
  }

  if (regexec(&regex, string_value, 2, matches, 0) == 0) {
    int start = matches[1].rm_so;
    int end = matches[1].rm_eo;

    strncpy(out_path, string_value + start, end - start);
    WriteToLogs("[Info] Opened  %s\n", out_path);
  } else {
    WriteToLogs("[Error] Couldn't Open  %s\n", out_path);
  }

  regfree(&regex);
}

void ParseHttpRequest(char* buffer, HttpRequestType* request) {
  void* http_loop[][2] = {
    {(void*)GET_HEADER, (void*)(intptr_t)HTTP_METHOD_GET},
    {(void*)POST_HEADER, (void*)(intptr_t)HTTP_METHOD_POST},
    {(void*)PUT_HEADER, (void*)(intptr_t)HTTP_METHOD_PUT},
    {(void*)DELETE_HEADER, (void*)(intptr_t)HTTP_METHOD_DELETE},
  };

  for (int i=0; i<4; i++) {
    if (strncmp(buffer, (char*)http_loop[i][0], strlen((char*)http_loop[i][0]))==0) {
      request->method = (int)(intptr_t)http_loop[i][1];
      ExtractPathFromReferer(buffer + strlen((char*)http_loop[i][0]), request->path);
      break;
    }
  }

  char* content_length_ptr = strstr(buffer, CONTENT_LENGTH_HEADER);
  if (content_length_ptr) {
    request->content_length = atoi(content_length_ptr+strlen(CONTENT_LENGTH_HEADER)); 
  }
  char* content_type_ptr = strstr(buffer, CONTENT_TYPE_HEADER);
  if (content_type_ptr)
  {
    char* end = strstr(content_type_ptr, "\r\n");
    if (end)
    {
      int len = end - (content_type_ptr + strlen(CONTENT_TYPE_HEADER));
      if (len < sizeof(request->content_type))
      {
        strncpy(request->content_type, content_type_ptr + strlen(CONTENT_TYPE_HEADER), len);
      }
    }
  }

  request->body = malloc(request->content_length+1);
  if (
    (request->method == HTTP_METHOD_POST || request->method == HTTP_METHOD_PUT) && 
    request->content_length > 0
  ) {
    char* body_start = strstr(buffer, "\r\n\r\n");
    memcpy(request->body, body_start, request->content_length);
    request->body[request->content_length] = '\0';
  }
}

int SanitizePaths(char* path) {
  if (strstr(path, "..") != NULL)
  {
    return -1;
  }

  return 0;
}

void HandleGetRequest(
  int client_fd,
  HttpRequestType* request
) {
  char response_header_buffer[BUFFER_SIZE];
  char response_body_buffer[BUFFER_SIZE];
  char file_path[BUFFER_SIZE];

  if (strcmp(request->path, "/") == 0)
  {
    snprintf(file_path, sizeof(file_path), "paths/index.html");
  } 
  else if (strstr(request->path, ".svg") || strstr(request->path, ".ico") || strstr(request->path, ".png"))
  {
    snprintf(file_path, sizeof(file_path), "assets%s", request->path);
  }
  else if (strstr(request->path, ".json"))
  {
    snprintf(file_path, sizeof(file_path), "api%s", request->path);
  }
  else if (strstr(request->path, "/api"))
  {
    DoPathHandle(
      client_fd,
      request,
      GET_REQUEST_HANDLER,
      GET_REQUEST_HANDLER_SIZE
    );
    return;
  }
  else
  {
    snprintf(file_path, sizeof(file_path), "paths%s.html", request->path);
  }
  
  FILE *file = fopen(file_path, "r");
  if (!file) {
    WriteToLogs("[Error]⚠️ Could not open %s\n", file_path);
    SendHTTPErrorResponse(client_fd, HTTP_NOT_FOUND);
    close(client_fd);
    return;
  }
  
  // Determine content type based on file extension
  const char* content_type = "text/html; charset=utf-8";
  if (strstr(file_path, ".css"))
    content_type = "text/css";
  else if (strstr(file_path, ".js"))
    content_type = "application/javascript";
  else if (strstr(file_path, ".png"))
    content_type = "image/png";
  else if (strstr(file_path, ".jpg") || strstr(file_path, ".jpeg"))
    content_type = "image/jpeg";
  else if (strstr(file_path, ".svg"))
    content_type = "image/svg+xml";
  else if (strstr(file_path, ".ico"))
    content_type = "image/x-icon";

  // Get all the files first
  fseek(file, 0, SEEK_END); 
  size_t file_size = ftell(file);
  rewind(file);
  
  GenerateResponseHeader(response_header_buffer, HTTP_OK, content_type, file_size);
  send(client_fd, response_header_buffer, strlen(response_header_buffer), 0);

  size_t bytes;
  while ((bytes = fread(response_body_buffer, 1, BUFFER_SIZE, file)) > 0)
  {
    send(client_fd, response_body_buffer, bytes, 0);
  }
  
  fclose(file);
  close(client_fd);
}

void DoPathHandle(
  int client_fd,
  HttpRequestType* request,
  PathToHandler* path_to_handler,
  size_t handler_size
) {

  int response = 0;
  for (int i = 0; i < handler_size; i++)
  {
    WriteToLogs(
      "TEST2\n, %s   %s", path_to_handler[i].path, request->path
    );
    if (strcmp(path_to_handler[i].path, request->path)==0) {
      path_to_handler[i].handler(client_fd, request);
      WriteToLogs(
        "Has done something?"
      );
      response = 1;
      break;
    }
  } 

  if (response != 1) {
    SendHTTPErrorResponse(client_fd, HTTP_BAD_REQUEST);
    WriteToLogs(
      "TEST3\n"
    );
    return;
  }
}

void HandlePostRequest(
  int client_fd,
  HttpRequestType* request
) {
  WriteToLogs(
    "POST request to %s with content-type %s and length %d\n",
    request->path, request->content_type, request->content_length
  );

  if (!request->body) {
    SendHTTPErrorResponse(client_fd, HTTP_BAD_REQUEST);
    return;
  }

  DoPathHandle(
    client_fd,
    request,
    POST_REQUEST_HANDLER,
    POST_REQUEST_HANDLER_SIZE
  );

  return;
}

void HandlePutRequest(
  int client_fd,
  HttpRequestType* request
) {
  WriteToLogs(
    "PUT request to %s with content-type %s and length %d\n",
    request->path, request->content_type, request->content_length
  );

  if (!request->body) {
    SendHTTPErrorResponse(client_fd, HTTP_BAD_REQUEST);
    return;
  }

  DoPathHandle(
    client_fd,
    request,
    PUT_REQUEST_HANDLER,
    PUT_REQUEST_HANDLER_SIZE
  );
}

void HandleDeleteRequest(
  int client_fd,
  HttpRequestType* request
) {
  WriteToLogs(
    "DELETE request to %s with content-type %s and length %d\n",
    request->path, request->content_type, request->content_length
  );

  if (!request->body) {
    SendHTTPErrorResponse(client_fd, HTTP_BAD_REQUEST);
    return;
  }

  DoPathHandle(
    client_fd,
    request,
    DELETE_REQUEST_HANDLER,
    DELETE_REQUEST_HANDLER_SIZE
  );

  return;
}

void HandleRequest(int client_fd) { 
  char header_buffer[BUFFER_SIZE];
  ssize_t total_bytes = 0;

  while (1) {
    ssize_t bytes_read = read(client_fd, header_buffer + total_bytes, BUFFER_SIZE - 1 - total_bytes);
    if (bytes_read == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No more data to read
        break;
      }
      perror("read");
      close(client_fd);
      return;
    } else if (bytes_read == 0) {
      // Client closed connection
      close(client_fd);
      return;
    }
    total_bytes += bytes_read;

    // Optional: prevent overflow
    if (total_bytes >= BUFFER_SIZE - 1) {
      break;
    }
  }

  HttpRequestType request = {0};
  ParseHttpRequest(header_buffer, &request);

  // Basic guard statements for requests.
  if (SanitizePaths(request.path) != 0)
  {
    SendHTTPErrorResponse(client_fd, HTTP_FORBIDDEN);
    close(client_fd);
    return;
  }

  switch(request.method) {
    case HTTP_METHOD_GET:
      WriteToLogs("GET REQUEST");
      HandleGetRequest(client_fd, &request);
      break;
    case HTTP_METHOD_POST:
      WriteToLogs("POST REQUEST");
      HandlePostRequest(client_fd, &request);
      break;
    case HTTP_METHOD_PUT:
      WriteToLogs("PUT REQUEST");
      HandlePutRequest(client_fd, &request);
      break;
    case HTTP_METHOD_DELETE:
      WriteToLogs("DELETE REQUEST");
      HandleDeleteRequest(client_fd, &request);
      break;
    default:
      WriteToLogs("[Error]: It is not GET, POST, PUT, or DELETE");
      break;
  }

  // Free requests and it is no longer needed.
  free(request.body);
}

void WriteRequestLog(HttpRequestType request) {
  WriteToLogs(
    "Request: \n"
    "method: %d,\n"
    "path: %s,\n"
    "body: %s,\n"
    "content_legnth: %s,\n"
    "content_type: %s\n", 
    request.method,
    request.path,
    request.body,
    request.content_length,
    request.content_type
  );
}


// TODO: Add types
void WriteToLogs(const char *restrict format, ...) {
  char logger_buffer[LOGGER_BUFFER] = {0};
  char timestamp[26];
  va_list args;

  va_start(args, format);
  vsnprintf(logger_buffer, LOGGER_BUFFER, format, args);

  struct stat file_stat = {0};
  if (stat("logs", &file_stat) == -1) {
    printf("logs folder do not exists");
    if (mkdir("logs", 0755) != 0) {
      printf("Cannot create log directory");
    }
  }

  FILE* file = fopen("logs/logers.txt", "a");
  GetTimeStamp(timestamp, sizeof(timestamp));

  printf("[%s] %s \n", timestamp, logger_buffer);
  fprintf(file, "[%s] %s \n", timestamp, logger_buffer);

  fclose(file);
}

void RunEpollLoop(const int server_fd) {
  struct epoll_event events_buffer[MAX_EVENTS];
  // EPOLLIN means ready for reading
  struct epoll_event ev = {
    .events = EPOLLIN,
    .data.fd = server_fd
  };
  int epfd = epoll_create1(0);

  epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);
  
  while (!stop_server) {
    // Find open events and overwrite to events 
    int n = epoll_wait(epfd, events_buffer, MAX_EVENTS, -1);
  
    // Look through all events  that are waiting
    for (int i = 0; i < n; i++) {
      int fd = events_buffer[i].data.fd;
  
      // If it is the server socket, it means a new client is trying to connect.
      if (fd == server_fd) {
        while (1) {
          // Create a new client socket where client can bind to socket_fd
          int client_fd = accept(server_fd, NULL, NULL);
          // No more connections to accept to given server
          // TODO: Maybe increase it on my own?
          if (client_fd == -1) break;    

          SetNonBlocking(client_fd);
  
          // Register this client socket with epoll.
          // using EPOLLET so that it notifies when the fd is ready?
          // not really sure about this one, but apprantly more performant
           struct epoll_event client_ev = {
            .events = EPOLLIN | EPOLLET,  
            .data.fd = client_fd
          };
          epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &client_ev);
        }
      } else {
        // If it is client, then handle the request. 
        HandleRequest(fd);
      }
    }
  }

  close(epfd);
}
