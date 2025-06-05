#ifndef MODEL_Persons
#define MODEL_Persons

#include <postgresql/libpq-fe.h>

typedef struct {
  char* personid;
  char* lastname;
  char* firstname;
  char* address;
  char* city;
} Persons;

typedef struct {
  Persons* Persons;
  ExecStatusType status;
} PersonsQuery;

PersonsQuery QueryPersons(PGconn* conn, const char* where_clause);
void InsertPersons(PGconn* conn, Persons u);
void UpdatePersons(PGconn* conn, Persons u, const char* where_clause);
void DeletePersons(PGconn* conn, const char* where_clause);

#endif // MODEL_Persons
