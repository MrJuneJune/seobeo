#ifndef POST_PATH
#include "server.h"

void handleFoo(int client_fd, HttpRequestType* request);
extern PathToHandler POST_REQUEST_HANDLER[];
extern size_t POST_REQUEST_HANDLER_SIZE;
extern PathToHandler DELETE_REQUEST_HANDLER[];
extern size_t POST_REQUEST_HANDLER_SIZE;
extern PathToHandler DELETE_REQUEST_HANDLER[];
extern size_t DELETE_REQUEST_HANDLER_SIZE;
extern PathToHandler PUT_REQUEST_HANDLER[];
extern size_t PUT_REQUEST_HANDLER_SIZE;

// TODO:
//  - Example using postgres would be nice as well.

#endif // POST_PATH
