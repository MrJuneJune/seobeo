#include <seobeo/helper.h>
#include <seobeo/server.h>
#include <sys/time.h>

// Global epoll file descriptor - needed for proper cleanup
static int global_epfd = -1;

// --- Response -- //
void GenerateResponseHeader
(
  char *buffer,
  int status,
  const char *content_type,
  const int content_length
) 
{
  const char *status_text;

  // TODO: Make this into compile time variables 
  switch(status)
  {
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
int SetNonBlocking(int fd)
{
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) return -1;
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void CreateSocket(int *server_fd)
{
  *server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (*server_fd == -1)
  {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }
  SetNonBlocking(*server_fd);
}

void BindToSocket(int *server_fd, struct sockaddr_in *server_addr)
{
  server_addr->sin_family = AF_INET;
  server_addr->sin_addr.s_addr = INADDR_ANY;
  server_addr->sin_port = htons(PORT);

  // Performance optimizations
  int opt = 1;
  setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  
  // Disable Nagle's algorithm for faster response times
  setsockopt(*server_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
  
  // Set larger socket buffers for better performance
  int buf_size = 65536;
  setsockopt(*server_fd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));
  setsockopt(*server_fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));

  if (bind(*server_fd, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0)
  {
    perror("bind failed");
    close(*server_fd);
    exit(EXIT_FAILURE);
  }
}

void ListenToSocket(int *server_fd)
{
  if (listen(*server_fd, SOMAXCONN) < 0)
  {
    perror("listen failed");
    close(*server_fd);
    exit(EXIT_FAILURE);
  }
  WriteToLogs("🚀 HTTP Server listening on port %d...\n", PORT);
}

void ExtractPathFromReferer(const char *string_value, char *out_path, char *out_query)
{
  regex_t regex;
  regmatch_t matches[2];
  const char *pattern = "(/[^ ?\r\n]*)";

  if (regcomp(&regex, pattern, REG_EXTENDED) != 0)
  {
    fprintf(stderr, "Failed to compile regex\n");
    return;
  }

  if (regexec(&regex, string_value, 2, matches, 0) == 0)
  {
    int start = matches[1].rm_so;
    int end = matches[1].rm_eo;

    strncpy(out_path, string_value + start, end - start);
    out_path[end - start] = '\0';

    const char *query_start = strchr(string_value + end, '?');
    if (query_start && strlen(query_start + 1) < MAX_QUERY_LEN)
    {
      strncpy(out_query, query_start + 1, MAX_QUERY_LEN - 1);
      out_query[MAX_QUERY_LEN - 1] = '\0';
    }
    else
    {
      out_query[0] = '\0'; // No query
    }

    WriteToLogs("[Info] Opened path: %s, query: %s", out_path, out_query);
  }
  else
  {
    WriteToLogs("[Error] Couldn't Open from: %s", string_value);
    out_path[0] = '\0';
    out_query[0] = '\0';
  }

  regfree(&regex);
}

void ParseHttpRequest(char *buffer, HttpRequestType *request)
{
  // Initialize request structure
  memset(request, 0, sizeof(HttpRequestType));
  
  void *http_loop[][2] = {
    {(void*)GET_HEADER, (void*)(intptr_t)HTTP_METHOD_GET},
    {(void*)POST_HEADER, (void*)(intptr_t)HTTP_METHOD_POST},
    {(void*)PUT_HEADER, (void*)(intptr_t)HTTP_METHOD_PUT},
    {(void*)DELETE_HEADER, (void*)(intptr_t)HTTP_METHOD_DELETE},
  };

  for (int i=0; i<4; i++)
  {
    if (strncmp(buffer, (char*)http_loop[i][0], strlen((char*)http_loop[i][0]))==0)
    {
      request->method = (int)(intptr_t)http_loop[i][1];
      ExtractPathFromReferer(buffer + strlen((char*)http_loop[i][0]), request->path, request->query);
      break;
    }
  }

  char *content_length_ptr = strstr(buffer, CONTENT_LENGTH_HEADER);
  if (content_length_ptr)
  {
    request->content_length = atoi(content_length_ptr+strlen(CONTENT_LENGTH_HEADER)); 
    // Sanitize content length - prevent huge allocations
    if (request->content_length > BUFFER_SIZE *10)
    {
      request->content_length = 0;
    }
  }
  
  char *content_type_ptr = strstr(buffer, CONTENT_TYPE_HEADER);
  if (content_type_ptr)
  {
    char *end = strstr(content_type_ptr, "\r\n");
    if (end)
    {
      int len = end - (content_type_ptr + strlen(CONTENT_TYPE_HEADER));
      if (len < sizeof(request->content_type) && len > 0)
      {
        strncpy(request->content_type, content_type_ptr + strlen(CONTENT_TYPE_HEADER), len);
        request->content_type[len] = '\0';
      }
    }
  }

  // Only allocate body if we have valid content length
  if (
    (request->method == HTTP_METHOD_POST || request->method == HTTP_METHOD_PUT) && 
    request->content_length > 0
  )
  {
    request->body = malloc(request->content_length + 4);
    if (!request->body)
    {
      WriteToLogs("[Error] Failed to allocate memory for request body");
      request->content_length = 0;
      return;
    }
    
    char *body_start = strstr(buffer, "\r\n\r\n");
    if (body_start)
    {
      body_start += 4;  // Skip past the CRLFCRLF to the actual body
      memcpy(request->body, body_start, request->content_length);
      request->body[request->content_length] = '\0';
    }
    else
    {
      // No body found, free the allocated memory
      free(request->body);
      request->body = NULL;
      request->content_length = 0;
    }
  }
  else
  {
    request->body = NULL;
  }
}

// TODO: Add more stuff since we only have basic values..
int SanitizePaths(char *path)
{
  if (!path) return -1;
  
  if (strstr(path, "..") != NULL)
  {
    return -1;
  }

  return 0;
}

int MatchRoute(const char *pattern, const char *path, HttpRequestType *req)
{
  const char *pattern_ptr = pattern;
  const char *path_ptr = path;
  req->path_params_len = 0;

  while (*pattern_ptr && *path_ptr)
  {
    if (*pattern_ptr == '{')
    {
      pattern_ptr++;
      char param_name[MAX_KEY_LEN] = {0};
      int i = 0;
      while (*pattern_ptr && *pattern_ptr != '}' && i < MAX_KEY_LEN - 1)
      {
        param_name[i++] = *pattern_ptr++;
      }
      if (*pattern_ptr != '}') return 0;
      pattern_ptr++;

      const char *next_slash = strchr(path_ptr, '/');
      int len = next_slash ? (next_slash - path_ptr) : strlen(path_ptr);
      if (len >= MAX_VALUE_LEN) return 0;

      strncpy(req->path_params[req->path_params_len].key, param_name, MAX_KEY_LEN - 1);
      strncpy(req->path_params[req->path_params_len].value, path_ptr, len);
      req->path_params[req->path_params_len].key[MAX_KEY_LEN - 1] = '\0';
      req->path_params[req->path_params_len].value[len] = '\0';
      req->path_params_len++;

      path_ptr += len;
    } 
    else
    {
      if (*pattern_ptr != *path_ptr) return 0;
      pattern_ptr++;
      path_ptr++;
    }
  }
  return *pattern_ptr == '\0' && *path_ptr == '\0';
}

void CleanupClient(int client_fd)
{
  // Disable Nagle's algorithm (optional for closed socket, but harmless)
  int opt = 1;
  setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

  if (global_epfd != -1) {
#if defined(__APPLE__) || defined(__FreeBSD__)
    // kqueue: remove fd
    struct kevent ev;
    EV_SET(&ev, client_fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    kevent(global_epfd, &ev, 1, NULL, 0, NULL);
#else
    // epoll
    epoll_ctl(global_epfd, EPOLL_CTL_DEL, client_fd, NULL);
#endif
  }

  close(client_fd);
}

int send_all(int sockfd, const void *buf, size_t len)
{
  const char *p = buf;
  size_t total_sent = 0;
  int retry_count = 0;
  const int max_retries = 3;

  while (total_sent < len)
  {
    ssize_t sent = send(sockfd, p + total_sent, len - total_sent, MSG_NOSIGNAL);
    
    if (sent < 0)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
      {
        // Socket buffer is full, wait a bit and retry
        if (retry_count < max_retries)
        {
          usleep(1000); // Wait 1ms
          retry_count++;
          continue;
        }
      }
      WriteToLogs("[Error] send() failed: %s\n", strerror(errno));
      return 0;
    }
    else if (sent == 0)
    {
      WriteToLogs("[Error] send() returned 0, client disconnected\n");
      return 0;
    }
    
    total_sent += sent;
    retry_count = 0; // Reset retry count on successful send
  }
  return 1;
}

void ServeStaticFileFallback(int client_fd, HttpRequestType *request)
{
  char file_path[BUFFER_SIZE] = {0};

  // Determine fallback path
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
  else if (strstr(request->path, ".html"))
  {
    snprintf(file_path, sizeof(file_path), "paths%s", request->path);
  }
  else
  {
    return; // No match, don't serve anything
  }

  // Check file existence and get size
  struct stat st;
  if (stat(file_path, &st) != 0)
  {
    WriteToLogs("[Error]⚠️ Could not stat %s\n", file_path);
    SendHTTPErrorResponse(client_fd, HTTP_NOT_FOUND);
    return;
  }
  size_t file_size = st.st_size;

  // Open file in binary mode
  FILE *file = fopen(file_path, "rb");
  if (!file)
  {
    WriteToLogs("[Error]⚠️ Could not open %s\n", file_path);
    SendHTTPErrorResponse(client_fd, HTTP_NOT_FOUND);
    return;
  }

  // Determine content type
  const char *content_type = "application/octet-stream"; // Default binary
  if (strstr(file_path, ".html")) content_type = "text/html; charset=utf-8";
  else if (strstr(file_path, ".css")) content_type = "text/css";
  else if (strstr(file_path, ".js")) content_type = "application/javascript";
  else if (strstr(file_path, ".png")) content_type = "image/png";
  else if (strstr(file_path, ".jpg") || strstr(file_path, ".jpeg")) content_type = "image/jpeg";
  else if (strstr(file_path, ".gif")) content_type = "image/gif";
  else if (strstr(file_path, ".svg")) content_type = "image/svg+xml";
  else if (strstr(file_path, ".ico")) content_type = "image/x-icon";
  else if (strstr(file_path, ".json")) content_type = "application/json";

  // Send headers
  char response_header_buffer[BUFFER_SIZE];
  GenerateResponseHeader(response_header_buffer, HTTP_OK, content_type, file_size);
  
  if (!send_all(client_fd, response_header_buffer, strlen(response_header_buffer)))
  {
    WriteToLogs("[Error]⚠️ Failed to send headers\n");
    fclose(file);
    return;
  }

  // Send file content in chunks
  char response_body_buffer[STATIC_FILE_BUFFER];
  size_t bytes_read;
  size_t total_bytes_sent = 0;

  while ((bytes_read = fread(response_body_buffer, 1, STATIC_FILE_BUFFER, file)) > 0)
  {
    if (!send_all(client_fd, response_body_buffer, bytes_read))
    {
      WriteToLogs("[Error]⚠️ Failed to send file data at byte %zu\n", total_bytes_sent);
      fclose(file);
      return;
    }
    total_bytes_sent += bytes_read;
  }

  // Verify we sent the expected amount
  if (total_bytes_sent != file_size)
  {
    WriteToLogs("[Warning]⚠️ Expected to send %zu bytes, actually sent %zu bytes\n", 
                file_size, total_bytes_sent);
  }

  fclose(file);
  WriteToLogs("[Info] Successfully served %s (%zu bytes)\n", file_path, total_bytes_sent);
}

void HandleRoutes(int client_fd, HttpRequestType *request, Route *routes, size_t route_count)
{
  for (size_t i = 0; i < route_count; i++)
  {
    if (routes[i].method != request->method) continue;
    if (strcmp(routes[i].path_pattern, request->path) == 0 ||
        MatchRoute(routes[i].path_pattern, request->path, request))
        {
      routes[i].handler(client_fd, request);
      CleanupClient(client_fd);
      return;
    }
  }

  if (request->method == HTTP_METHOD_GET)
  {
    ServeStaticFileFallback(client_fd, request);
    CleanupClient(client_fd);
    return;
  }

  SendHTTPErrorResponse(client_fd, HTTP_NOT_FOUND);
  CleanupClient(client_fd);
}

void HandleRequest(int client_fd)
{
  // Add timing to debug performance
  struct timeval start, end;
  gettimeofday(&start, NULL);
  
  char header_buffer[BUFFER_SIZE];
  ssize_t total_bytes = 0;

  while (1)
  {
    // Check if we're about to overflow before reading
    if (total_bytes >= BUFFER_SIZE - 1)
    {
      WriteToLogs("[Warning] Request too large, truncating");
      break;
    }
    
    ssize_t max_read = BUFFER_SIZE - 1 - total_bytes;
    ssize_t bytes_read = read(client_fd, header_buffer + total_bytes, max_read);
    
    if (bytes_read == -1)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
      {
        // No more data to read
        break;
      }
      perror("read");
      CleanupClient(client_fd);
      return;
    }
    else if (bytes_read == 0)
    {
      // Client closed connection
      CleanupClient(client_fd);
      return;
    }
    total_bytes += bytes_read;
  }

  // Null terminate the buffer
  header_buffer[total_bytes] = '\0';

  HttpRequestType request = {0};
  ParseHttpRequest(header_buffer, &request);

  // Basic guard statements for requests.
  if (SanitizePaths(request.path) != 0)
  {
    SendHTTPErrorResponse(client_fd, HTTP_FORBIDDEN);
    CleanupClient(client_fd);
    goto cleanup;
  }

  WriteRequestLog(request);
  HandleRoutes(client_fd, &request, ROUTE, ROUTE_SIZE);

cleanup:
  // Free request body if allocated
  if (request.body)
  {
    free(request.body);
  }
  
  // Log timing
  gettimeofday(&end, NULL);
  long diff = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
  WriteToLogs("[TIMING] Request took %ld ms", diff);
}

void WriteRequestLog(HttpRequestType request)
{
  WriteToLogs(
    "[INFO] HTTP Request:\n"
    "  Method        : %d\n"
    "  Path          : %s\n"
    "  Content-Type  : %s\n"
    "  Content-Length: %d\n"
    "  Body          : %s\n"
    "  Query         : %s\n"
    "  Path Params   :",
    request.method,
    request.path[0] ? request.path : "/",
    request.content_type[0] ? request.content_type : "N/A",
    request.content_length,
    (strstr(request.path, "api") == 0) ? (request.body ? request.body : "N/A") : "N/A",
    request.query[0] ? request.query : "N/A"
  );

  if (request.path_params_len == 0)
  {
    WriteToLogs("    (none)");
  }
  else
  {
    for (size_t i = 0; i < request.path_params_len; ++i)
    {
      WriteToLogs("    - %s: %s",
        request.path_params[i].key,
        request.path_params[i].value
      );
    }
  }
}

// TODO: Add types
void WriteToLogs(const char *restrict format, ...)
{
  char logger_buffer[LOGGER_BUFFER] = {0};
  char timestamp[26];
  va_list args;

  va_start(args, format);
  vsnprintf(logger_buffer, LOGGER_BUFFER, format, args);
  va_end(args);

  struct stat file_stat = {0};
  if (stat("logs", &file_stat) == -1)
  {
    printf("logs folder do not exists");
    if (mkdir("logs", 0755) != 0)
    {
      printf("Cannot create log directory");
    }
  }

  FILE *file = fopen("logs/logers.txt", "a");
  if (file)
  {
    GetTimeStamp(timestamp, sizeof(timestamp));
    printf("[%s] %s \n", timestamp, logger_buffer);
    fprintf(file, "[%s] %s \n", timestamp, logger_buffer);
    fclose(file);
  }
}

void RunEpollLoop(const int server_fd)
{
  #if defined(__APPLE__) || defined(__FreeBSD__)
  struct kevent change_event;
  struct kevent events_buffer[MAX_EVENTS];

  int kq = kqueue();
  if (kq == -1) {
    perror("kqueue");
    return;
  }

  global_epfd = kq;

  EV_SET(&change_event, server_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
  if (kevent(kq, &change_event, 1, NULL, 0, NULL) == -1) {
    perror("kevent register");
    close(kq);
    return;
  }

  while (!stop_server) {
    int n = kevent(kq, NULL, 0, events_buffer, MAX_EVENTS, NULL);
    if (n == -1) {
      if (errno == EINTR) continue;
      perror("kevent");
      break;
    }

    for (int i = 0; i < n; i++) {
      int fd = (int)events_buffer[i].ident;

      if (fd == server_fd) {
        while (1) {
          int client_fd = accept(server_fd, NULL, NULL);
          if (client_fd == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
              perror("accept");
            }
            break;
          }

          if (SetNonBlocking(client_fd) == -1) {
            perror("SetNonBlocking");
            close(client_fd);
            continue;
          }

          int opt = 1;
          setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));

          EV_SET(&change_event, client_fd, EVFILT_READ, EV_ADD | EV_CLEAR, 0, 0, NULL);
          if (kevent(kq, &change_event, 1, NULL, 0, NULL) == -1) {
            perror("kevent add client");
            close(client_fd);
            continue;
          }
        }
      } else {
        HandleRequest(fd);
      }
    }
  }

  close(kq);
  global_epfd = -1;
  #else
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
  global_epfd = epfd;

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
        // If it is client, then handle the request.
        // Note: CleanupClient will remove from epoll and close the fd
        HandleRequest(fd);
      }
    }
  }

  close(epfd);
  global_epfd = -1;
  #endif
}
