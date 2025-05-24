#include <seobeo/router.h>

void handleFoo(int client_fd, HttpRequestType* request) {
  char response_header_buffer[BUFFER_SIZE];
  const char* content_type = "application/json";
  char* response = "{\"foo\": \"bar\"}";

  GenerateResponseHeader(response_header_buffer, HTTP_OK, content_type, strlen(response));
  send(client_fd, response_header_buffer, strlen(response_header_buffer), 0);
  send(client_fd,  response, strlen(response), 0);
  return;
}

PathToHandler POST_REQUEST_HANDLER[] = {
  {
    "api/foo",
    &handleFoo
  }
};

size_t POST_REQUEST_HANDLER_SIZE =  1;


PathToHandler DELETE_REQUEST_HANDLER[] = {
  {
    "api/foo",
    &handleFoo
  }
};

size_t DELETE_REQUEST_HANDLER_SIZE =  1;

PathToHandler PUT_REQUEST_HANDLER[] = {
  {
    "api/foo",
    &handleFoo
  }
};

size_t PUT_REQUEST_HANDLER_SIZE =  1;
