#include "model_Persons.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

PersonsQuery QueryPersons(PGconn* conn, const char* where_clause)
{
  PersonsQuery query_result;
  char query[1024];
  snprintf(query, sizeof(query), "SELECT * FROM Persons WHERE %s;", where_clause);
  PGresult* res = PQexec(conn, query);
  ExecStatusType status = PQresultStatus(res);  query_result.Persons = NULL;
  query_result.status = status;  if (status != PGRES_TUPLES_OK)
  {
    fprintf(stderr, "SELECT failed: %s\n", PQerrorMessage(conn));
    PQclear(res);
    return query_result;
  }
  int rows = PQntuples(res);
   if (rows == 0)
   {
     return query_result;
   }
  Persons* list = malloc(rows * sizeof(Persons));
  for (int i = 0; i < rows; ++i)
  {
    list[i].personid = strdup(PQgetvalue(res, i, 0));
    list[i].lastname = strdup(PQgetvalue(res, i, 1));
    list[i].firstname = strdup(PQgetvalue(res, i, 2));
    list[i].address = strdup(PQgetvalue(res, i, 3));
    list[i].city = strdup(PQgetvalue(res, i, 4));
  }
  PQclear(res);
  query_result.Persons = list;  return query_result;}

void InsertPersons(PGconn* conn, Persons u)
{
  char query[1024];
  snprintf(query, sizeof(query),
    "INSERT INTO Persons (personid, lastname, firstname, address, city) "
    "VALUES ('%s', '%s', '%s', '%s', '%s');",
    u.personid, u.lastname, u.firstname, u.address, u.city);
  PGresult* res = PQexec(conn, query);
  PQclear(res);
}

void UpdatePersons(PGconn* conn, Persons u, const char* where_clause)
{
  char query[1024];
  snprintf(query, sizeof(query),
    "UPDATE Persons "
    "SET personid='%s', lastname='%s', firstname='%s', address='%s', city='%s' WHERE %s;",
    u.personid, u.lastname, u.firstname, u.address, u.city, where_clause);
  PGresult* res = PQexec(conn, query);
  PQclear(res);
}

void DeletePersons(PGconn* conn, const char* where_clause)
{
  char query[1024];
  snprintf(query, sizeof(query),
    "DELETE FROM Persons WHERE %s;",
    where_clause);
  PGresult* res = PQexec(conn, query);
  PQclear(res);
}
