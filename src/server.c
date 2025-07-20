#include <seobeo/helper.h>
#include <seobeo/server.h>
#include <seobeo/os.h>
#include <sys/time.h>

FILE  *g_log_file;

void InitGlobalVariables()
{
    mkdir("logs", 0755);
    g_log_file = fopen("logs/loggers.txt", "a");
    if (!g_log_file) {
      perror("fopen"); exit(1);
    }
    setvbuf(g_log_file, NULL, _IOLBF, 0);
}

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

  WriteToLogs(
    "Response:\n"
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
  int buf_size = 65536 * 24;
  setsockopt(*server_fd, SOL_SOCKET, SO_RCVBUF, &buf_size, sizeof(buf_size));
  setsockopt(*server_fd, SOL_SOCKET, SO_SNDBUF, &buf_size, sizeof(buf_size));

  #ifdef SO_REUSEPORT
  setsockopt(*server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
  #endif

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

void ExtractPathFromReferer(
    const char *src,
    char *out_path,
    char *out_query
)
{
    const char *p        = src; 
    const char *path_end = NULL;
    const char *q_start  = NULL;

    for (; *p && *p != '\r' && *p != '\n'; ++p) {
        if (*p == ' ') {
            if (!path_end) path_end = p;
            break;
        }
        if (*p == '?' && !q_start) {
            path_end = p;
            q_start  = p + 1;
        }
    }
    if (!path_end) path_end = p;

    size_t path_len = (size_t)(path_end - src);
    if (path_len >= MAX_PATH_LEN) path_len = MAX_PATH_LEN - 1;
    memcpy(out_path, src, path_len);
    out_path[path_len] = '\0';

    if (q_start)
    {
        const char *q_end = strchr(q_start, ' ');
        if (!q_end) q_end = q_start + strlen(q_start);
        size_t q_len = (size_t)(q_end - q_start);
        if (q_len >= MAX_QUERY_LEN) q_len = MAX_QUERY_LEN - 1;
        memcpy(out_query, q_start, q_len);
        out_query[q_len] = '\0';
    }
    else
    {
        out_query[0] = '\0';
    }
}

void ParseHttpRequest(char *buffer, HttpRequestType *request, Arena *request_arena)
{  
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
    request->body = ArenaAlloc(request_arena, request->content_length + 4);
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

int SendAll(int sockfd, const void *buf, size_t len)
{
  const char *p = buf;
  size_t total_sent = 0;
  int retry_count = 0;
  const int max_retries = 100;
  const int sleep_us = 5000;

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
          usleep(sleep_us); 
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
  StaticFileEntry *entry;

  if (0)
  {
    entry = GetHashMapValue(static_file, file_path);
    if (!entry)
    {
      WriteToLogs("Cache miss : %s\n", file_path);
      entry = LoadStaticFile(file_path, content_type);
    }
    else 
    {
      WriteToLogs("Cache hit : %s\n", file_path);
    }
  }
  // Turn off cache
  else
  {
    entry = LoadStaticFile(file_path, content_type);
  }

  if (!entry)
  {
    SendHTTPErrorResponse(client_fd, HTTP_NOT_FOUND);
    return;
  }

  char response_header_buffer[BUFFER_SIZE];
  GenerateResponseHeader(response_header_buffer, HTTP_OK, entry->content_type, entry->size);
  if (!SendAll(client_fd, response_header_buffer, strlen(response_header_buffer)))
  {
    WriteToLogs("[Error]⚠️ Failed to send headers for %s\n", file_path);
    return;
  }

  size_t total_sent = 0;
  while (total_sent < entry->size)
  {
    size_t chunk = STATIC_FILE_BUFFER;
    if (entry->size - total_sent < STATIC_FILE_BUFFER)
      chunk = entry->size - total_sent;

    if (!SendAll(client_fd, entry->data + total_sent, chunk))
    {
      WriteToLogs("[Error]⚠️ Failed to send data at byte %zu\n", total_sent);
      return;
    }
    total_sent += chunk;
  }

  WriteToLogs("[Info]✅ Served %s from memory (%zu bytes)\n", file_path, entry->size);
}

StaticFileEntry *LoadStaticFile(const char *file_path, const char *content_type)
{

  printf("file open");
  FILE *file = fopen(file_path, "rb");
  if (!file) return NULL;

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *data = malloc(size);
  fread(data, 1, size, file);
  fclose(file);

  StaticFileEntry *entry = malloc(sizeof(StaticFileEntry));
  entry->data = data;
  entry->size = size;
  entry->content_type = content_type;

  InsertHashMap(static_file, file_path, entry);

  return entry;
}

void FreeStaticFileEntry(void *entry_ptr)
{
  StaticFileEntry *entry = (StaticFileEntry *)entry_ptr;
  if (!entry)
  {
    return;
  }
  if (entry->data)
  {
    free(entry->data);
  }
  free(entry);
}

void HandleRoutes(int client_fd, HttpRequestType *request, Route *routes, size_t route_count)
{
  // Checks for API first
  // TODO: Maybe make a simple route check so we don't need to do both?.
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

  // If it is GET, we static file fallback.
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

  Arena *request_arena = ArenaCreate(REQUEST_ARENA_SIZE);

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

  HttpRequestType *request = ArenaAlloc(request_arena, sizeof(HttpRequestType));
  if (request) *request = (HttpRequestType){0};
  ParseHttpRequest(header_buffer, request, request_arena);

  // Basic guard statements for requests.
  if (SanitizePaths((*request).path) != 0)
  {
    SendHTTPErrorResponse(client_fd, HTTP_FORBIDDEN);
    CleanupClient(client_fd);
    goto cleanup;
  }

  WriteRequestLog(*request);
  HandleRoutes(client_fd, request, ROUTE, ROUTE_SIZE);

cleanup:
  ArenaDestroy(request_arena);
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
void WriteToLogs(const char *fmt, ...)
{
    char buf[LOGGER_BUFFER];
    char ts[32];

    va_list ap; va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);

    struct timespec tv; clock_gettime(CLOCK_REALTIME, &tv);
    struct tm tm; gmtime_r(&tv.tv_sec, &tm);         /* lock-free variant */
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", &tm);

    fprintf(g_log_file, "[%s.%03ld] %s\n",
            ts, tv.tv_nsec / 1000000, buf);
    printf( "[%s.%03ld] %s\n",
            ts, tv.tv_nsec / 1000000, buf);
}

void CreateHTTPResponse(int client_fd, char *response, const char *content_type, char *response_header_buffer)
{
  size_t total_size = strlen(response);
  size_t bytes_sent = 0;

  GenerateResponseHeader(response_header_buffer, HTTP_OK, content_type, total_size);
  send(client_fd, response_header_buffer, strlen(response_header_buffer), 0);

  while (total_size > 0)
  {
    size_t chunk_size = total_size > STATIC_FILE_BUFFER ? STATIC_FILE_BUFFER : total_size;
    memcpy(response_header_buffer, response + bytes_sent, chunk_size);

    if (!SendAll(client_fd, response_header_buffer, chunk_size))
    {
      WriteToLogs("[Error]⚠️ Failed to send file data at byte %zu\n", bytes_sent);
      return;
    }

    bytes_sent += chunk_size;
    total_size -= chunk_size;
  }
}

void InitQueue(ClientQueue *q)
{
    q->head = q->tail = q->count = 0;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_empty, NULL);
}

void Enqueue(ClientQueue *q, int fd)
{
    pthread_mutex_lock(&q->mutex);
    if (q->count == QUEUE_CAPACITY) {
        fprintf(stderr, "Queue full, dropping client_fd %d\n", fd);
        close(fd);
    } else {
        q->fds[q->tail] = fd;
        q->tail = (q->tail + 1) % QUEUE_CAPACITY;
        q->count++;
        pthread_cond_signal(&q->not_empty);
    }
    pthread_mutex_unlock(&q->mutex);
}

int Dequeue(ClientQueue *q)
{
    pthread_mutex_lock(&q->mutex);
    while (q->count == 0) {
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }
    int fd = q->fds[q->head];
    q->head = (q->head + 1) % QUEUE_CAPACITY;
    q->count--;
    pthread_mutex_unlock(&q->mutex);
    return fd;
}

void* WorkerThread(void* arg) {
    ClientQueue* queue = (ClientQueue*)arg;

    while (1) {
        int fd = Dequeue(queue);
        HandleRequest(fd);
    }

    return NULL;
}

