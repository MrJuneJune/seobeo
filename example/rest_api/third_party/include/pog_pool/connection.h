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

void InitPool(volatile ConnectionPool *pool, const char *conninfo);
PGconn *BorrowConnection(volatile ConnectionPool *pool); 
void ReleaseConnection(volatile ConnectionPool *pool, PGconn *conn); 
void ClosePool(volatile ConnectionPool *pool); 
void RunSQLFile(PGconn *conn, const char *filename);

#endif // CONNECTION
