#include "connection.h" 

void InitPool(volatile ConnectionPool *pool, const char *conninfo)
{
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    pool->connections[i] = NULL;
  }
  pool->num_connections = 0;
  
  for (int i = 0; i < MAX_CONNECTIONS; i++) {
    pool->connections[i] = PQconnectdb(conninfo);
    if (PQstatus(pool->connections[i]) != CONNECTION_OK) {
      printf("Connection to database failed: %s\n", PQerrorMessage(pool->connections[i]));
      exit(1);
    }
    pool->num_connections++;
  }
}

PGconn* BorrowConnection(volatile ConnectionPool *pool)
{
  if (pool->num_connections == 0)
  {
    printf("No available connections in the pool.\n");
    return NULL;
  }
  PGconn *conn = pool->connections[0];
  
  for (int i = 0; i < pool->num_connections - 1; i++) 
  {
    pool->connections[i] = pool->connections[i + 1];
  }
  pool->num_connections--;
  
  return conn;
}

void ReleaseConnection(volatile ConnectionPool *pool, PGconn *conn)
{
  if (pool->num_connections >= MAX_CONNECTIONS)
  {
    printf("Pool is full. Cannot release connection.\n");
    return;
  }
  pool->connections[pool->num_connections] = conn;
  pool->num_connections++;
}

void ClosePool(volatile ConnectionPool *pool)
{
  for (int i = 0; i < pool->num_connections; i++) {
    PQfinish(pool->connections[i]);
  }
  pool->num_connections = 0;
}

