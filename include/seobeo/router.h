#ifndef POST_PATH
#include "server.h"

void handleFoo(int client_fd, HttpRequestType* request);
extern PathToHandler POST_REQUEST_HANDLER[];
extern size_t POST_REQUEST_HANDLER_SIZE;

// TODO:
//  - Add for PUT, DELETE, and others basically copy and pasate?
//  - Example using postgres would be nice as well.

#endif // POST_PATH
