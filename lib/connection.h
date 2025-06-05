#ifndef CONNECTION
#define CONNECTION
#include <stdio.h>
#include <stdlib.h>
#include <postgresql/libpq-fe.h>

#define MAX_CONNECTIONS 10

typedef struct {
  PGconn *connections[MAX_CONNECTIONS];
  int num_connections;
} ConnectionPool;

void InitPool(ConnectionPool *pool, const char *conninfo);
PGconn* BorrowConnection(ConnectionPool *pool); 
void ReleaseConnection(ConnectionPool *pool, PGconn *conn); 
void ClosePool(ConnectionPool *pool); 

#endif // CONNECTION
