#include <sys/socket.h>

void CreateSocket(int* server_fd);
void BindToSocket(int* server_fd, struct sockaddr_in* server_addr);
void ListenToSocket(int* server_fd);
int SetupEpoll(int server_fd);
void HandleRequest(int client_fd);
