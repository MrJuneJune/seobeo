#ifndef OS_HEADER
#define OS_HEADER

#if defined(__APPLE__) || defined(__FreeBSD__)
#define MACOS
#else
#define LINUX
#endif

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/tcp.h>

void CleanupClient(int client_fd);
void RunEventLoop(int server_fd);
#endif // OS_HEADER
