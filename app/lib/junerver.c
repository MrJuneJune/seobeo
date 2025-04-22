#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "junerver.h"
#include "juneper.h"

// TODO: Nice to have this as a compile time variables.
void GenerateResponseHeader(char* buffer, int status, const char* content_type) {
  const char* status_text;
  
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
  
  sprintf(buffer, 
      "HTTP/1.1 %d %s\r\n"
      "Content-Type: %s\r\n"
      "Connection: close\r\n"
      "\r\n", 
      status, status_text, content_type);
}

void SendHTTPErrorResponse(int client_fd, int status_code) {
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
  
  GenerateResponseHeader(header, status_code, "text/html");
  
  send(client_fd, header, strlen(header), 0);
  send(client_fd, body, strlen(body), 0);
}

int SetNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
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
  printf("🚀 HTTP Server listening on port %d...\n", PORT);
}

void HandleRequest(int client_fd) {
  char header_buffer[BUFFER_SIZE] = {0};
  char response_body_buffer[BUFFER_SIZE] = {0};
  char response_header_buffer[BUFFER_SIZE] = {0};
  char path[1024] = {0};
  char file_path[1024] = {0};
  
  ssize_t bytes_read = read(client_fd, header_buffer, BUFFER_SIZE - 1);

  // No request being sent.
  if (bytes_read <= 0) {
    close(client_fd);
    return;
  }

  int start_pos = FindChildrenFromParentGeneric(
    header_buffer, BUFFER_SIZE,
    GET_HEADER, strlen(GET_HEADER),
    sizeof(char)
  );
  
  if (start_pos != -1) {
    ExtractPathFromReferer(header_buffer + start_pos, path);
  } else {
    SendHTTPErrorResponse(client_fd, HTTP_BAD_REQUEST);
    close(client_fd);
    return;
  }
  
  // Basic path sanitization to prevent directory traversal
  if (strstr(path, "..") != NULL) {
    SendHTTPErrorResponse(client_fd, HTTP_FORBIDDEN);
    close(client_fd);
    return;
  }
  
  if (strcmp(path, "/") == 0) {
    snprintf(file_path, sizeof(file_path), "paths/index.html");
  } else if (strstr(path, ".svg") || strstr(path, ".ico") || strstr(path, ".png")) {
    snprintf(file_path, sizeof(file_path), "assets%s", path);
  } else {
    snprintf(file_path, sizeof(file_path), "paths%s.html", path);
  }
  
  FILE *file = fopen(file_path, "r");
  if (!file) {
    printf("⚠️ Could not open %s\n", file_path);
    SendHTTPErrorResponse(client_fd, HTTP_NOT_FOUND);
    close(client_fd);
    return;
  }
  
  // Determine content type based on file extension
  const char* content_type = "text/html; charset=utf-8 ";
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
  
  GenerateResponseHeader(response_header_buffer, HTTP_OK, content_type);
  send(client_fd, response_header_buffer, strlen(response_header_buffer), 0);
  
  while (fgets(response_body_buffer, BUFFER_SIZE, file)) {
    send(client_fd, response_body_buffer, strlen(response_body_buffer), 0);
  }
  
  fclose(file);
  close(client_fd);
}
