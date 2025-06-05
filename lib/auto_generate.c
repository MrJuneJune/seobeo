#include "auto_generate.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>



const char* SqlToCType(const char* sql_type)
{
  if (strncmp(sql_type, "INT", 3) == 0 || strncmp(sql_type, "INTEGER", 7) == 0) return "int";
  if (strncmp(sql_type, "SMALLINT", 8) == 0) return "short";
  if (strncmp(sql_type, "BIGINT", 6) == 0) return "long long";
  if (strncmp(sql_type, "UUID", 4) == 0) return "char*";
  if (strncmp(sql_type, "VARCHAR", 7) == 0) return "char*";
  if (strncmp(sql_type, "CHAR", 4) == 0) return "char*";
  if (strncmp(sql_type, "TEXT", 4) == 0) return "char*";
  if (strncmp(sql_type, "TIMESTAMP", 9) == 0) return "char*";
  if (strncmp(sql_type, "DATE", 4) == 0) return "char*";
  if (strncmp(sql_type, "TIME", 4) == 0) return "char*";
  if (strncmp(sql_type, "BOOLEAN", 7) == 0 || strncmp(sql_type, "BOOL", 4) == 0) return "int";
  if (strncmp(sql_type, "FLOAT", 5) == 0) return "float";
  if (strncmp(sql_type, "DOUBLE", 6) == 0) return "double";
  if (strncmp(sql_type, "DECIMAL", 7) == 0 || strncmp(sql_type, "NUMERIC", 7) == 0) return "double";
  if (strncmp(sql_type, "JSON", 4) == 0 || strncmp(sql_type, "JSONB", 5) == 0) return "void*";
  if (strncmp(sql_type, "BYTEA", 5) == 0) return "void*";
  if (strncmp(sql_type, "ARRAY", 5) == 0) return "void*";

  // Default fallback
  return "void*";
}

char* SkipSpaces(char* p)
{
  while (*p == ' ' || *p == '\t') p++;
  return p;
}

int GetWord(char* line, char* word)
{
  int res = 0;
  while (*line != ' ' && *line != ',' && *line != '\0')
  {
    *word = *line;
    line++; 
    word++;
    res++;
  }
  *word = '\0';
  return res;
}

void CreateCRUDFiles(
  const char* table_name,
  const Column* columns,
  size_t column_sizes
)
{
  // TODO: Make this dynamic by counting length of the file?
  char file_name[512];

  // Create header file
  snprintf(file_name, sizeof(file_name), "/home/mrjunejune/project/pog_pool/lib/model_%s.h", table_name);
  FILE *file_out_p = fopen(file_name, "w");
  if (!file_out_p)
  {
    perror("fopen");
    return;
  }

  fprintf(file_out_p, "#ifndef MODEL_%s\n", table_name);
  fprintf(file_out_p, "#define MODEL_%s\n\n", table_name);
  fprintf(file_out_p, "#include <postgresql/libpq-fe.h>\n\n");
  fprintf(file_out_p, "typedef struct {\n");
  for (size_t i = 0; i < column_sizes; i++)
  {
    const char *c_type = SqlToCType(columns[i].type);
    fprintf(file_out_p, "  %s %s;\n", c_type, columns[i].name);
  }
  fprintf(file_out_p, "} %s;\n\n",table_name);

  fprintf(file_out_p, "typedef struct {\n");
  fprintf(file_out_p, "  %s* %s;\n", table_name, table_name);
  fprintf(file_out_p, "  ExecStatusType status;\n");
  fprintf(file_out_p, "} %sQuery;\n\n", table_name);

  fprintf(file_out_p, "%sQuery Query%s(PGconn* conn, const char* where_clause);\n", table_name, table_name);
  fprintf(file_out_p, "void Insert%s(PGconn* conn, %s u);\n", table_name, table_name);
  fprintf(file_out_p, "void Update%s(PGconn* conn, %s u, const char* where_clause);\n", table_name, table_name);
  fprintf(file_out_p, "void Delete%s(PGconn* conn, const char* where_clause);\n\n", table_name);
  fprintf(file_out_p, "#endif // MODEL_%s\n", table_name);
  fclose(file_out_p);

  // Create source file 
  snprintf(file_name, sizeof(file_name), "/home/mrjunejune/project/pog_pool/lib/model_%s.c", table_name);
  file_out_p = fopen(file_name, "w");
  if (!file_out_p)
  {
    perror("fopen");
    return;
  }

  // Build column list and format string
  char column_names[512] = {0};
  char format_parts[512] = {0};
  char value_args[512] = {0};

  strcat(column_names, "(");
  for (size_t i = 0; i < column_sizes; i++)
  {
    const char* name = columns[i].name;
    const char* c_type = SqlToCType(columns[i].type);

    strcat(column_names, name);
    if (strcmp(c_type, "int") == 0 || strcmp(c_type, "short") == 0 || strcmp(c_type, "long long") == 0) {
      strcat(format_parts, "%d");
    } else if (strcmp(c_type, "float") == 0 || strcmp(c_type, "double") == 0) {
      strcat(format_parts, "%f");
    } else {
      strcat(format_parts, "'%s'");
    }

    char tmp[64];
    snprintf(tmp, sizeof(tmp), "u.%s", name);
    strcat(value_args, tmp);

    if (i != column_sizes - 1) {
      strcat(column_names, ", ");
      strcat(format_parts, ", ");
      strcat(value_args, ", ");
    }
  }
  strcat(column_names, ")");

  // Build SET clause and format string
  char set_clause[512] = {0};
  char update_args[512] = {0};
  for (size_t i = 0; i < column_sizes; i++)
  {
    const char* name = columns[i].name;
    const char* c_type = SqlToCType(columns[i].type);

    char set_line[64];
    if (strcmp(c_type, "int") == 0 || strcmp(c_type, "short") == 0 || strcmp(c_type, "long long") == 0) {
      snprintf(set_line, sizeof(set_line), "%s=%%d", name);
    } else if (strcmp(c_type, "float") == 0 || strcmp(c_type, "double") == 0) {
      snprintf(set_line, sizeof(set_line), "%s=%%f", name);
    } else {
      snprintf(set_line, sizeof(set_line), "%s='%%s'", name);
    }
    strcat(set_clause, set_line);

    char arg_line[64];
    snprintf(arg_line, sizeof(arg_line), "u.%s", name);
    strcat(update_args, arg_line);

    if (i != column_sizes - 1) {
      strcat(set_clause, ", ");
      strcat(update_args, ", ");
    }
  }

  fprintf(file_out_p, "#include \"model_%s.h\"\n", table_name);
  fprintf(file_out_p, "#include <stdlib.h>\n#include <stdio.h>\n#include <string.h>\n\n");

  // QUERY function
  fprintf(file_out_p, "%sQuery Query%s(PGconn* conn, const char* where_clause)\n{\n", table_name, table_name);
  fprintf(file_out_p, "  %sQuery query_result;\n", table_name);
  fprintf(file_out_p, "  char query[%i];\n", QUERY_BUFFER);
  fprintf(file_out_p, "  snprintf(query, sizeof(query), \"SELECT * FROM %s WHERE %%s;\", where_clause);\n", table_name);
  fprintf(file_out_p, "  PGresult* res = PQexec(conn, query);\n");
  fprintf(file_out_p, "  ExecStatusType status = PQresultStatus(res);");
  fprintf(file_out_p, "  query_result.%s = NULL;\n", table_name);
  fprintf(file_out_p, "  query_result.status = status;");
  fprintf(file_out_p, "  if (status != PGRES_TUPLES_OK)\n  {\n");
  fprintf(file_out_p, "    fprintf(stderr, \"SELECT failed: %%s\\n\", PQerrorMessage(conn));\n");
  fprintf(file_out_p, "    PQclear(res);\n");
  fprintf(file_out_p, "    return query_result;\n");
  fprintf(file_out_p, "  }\n");
  
  fprintf(file_out_p, "  int rows = PQntuples(res);\n");

  fprintf(file_out_p, "   if (rows == 0)\n");
  fprintf(file_out_p, "   {\n");
  fprintf(file_out_p, "     return query_result;\n");
  fprintf(file_out_p, "   }\n");
  fprintf(file_out_p, "  %s* list = malloc(rows * sizeof(%s));\n", table_name, table_name);
  fprintf(file_out_p, "  for (int i = 0; i < rows; ++i)\n  {\n");
  
  for (size_t i = 0; i < column_sizes; i++) {
    const char* name = columns[i].name;
    const char* c_type = SqlToCType(columns[i].type);
  
    if (strcmp(c_type, "int") == 0 || strcmp(c_type, "short") == 0 || strcmp(c_type, "long long") == 0) {
      fprintf(file_out_p, "    list[i].%s = atoi(PQgetvalue(res, i, %zu));\n", name, i);
    } else if (strcmp(c_type, "float") == 0 || strcmp(c_type, "double") == 0) {
      fprintf(file_out_p, "    list[i].%s = atof(PQgetvalue(res, i, %zu));\n", name, i);
    } else {
      fprintf(file_out_p, "    list[i].%s = strdup(PQgetvalue(res, i, %zu));\n", name, i);
    }
  }
  
  fprintf(file_out_p, "  }\n");
  fprintf(file_out_p, "  PQclear(res);\n");
  fprintf(file_out_p, "  query_result.%s = list;", table_name);
  fprintf(file_out_p, "  return query_result;");
  fprintf(file_out_p, "}\n\n");

  // INSERT function
  fprintf(file_out_p, "void Insert%s(PGconn* conn, %s u)\n{\n", table_name, table_name);
  fprintf(file_out_p, "  char query[%i];\n", QUERY_BUFFER);
  fprintf(file_out_p, "  snprintf(query, sizeof(query),\n");
  fprintf(file_out_p, "    \"INSERT INTO %s %s \"\n    \"VALUES (%s);\",\n", table_name, column_names, format_parts);
  fprintf(file_out_p, "    %s);\n", value_args);
  fprintf(file_out_p, "  PGresult* res = PQexec(conn, query);\n");
  fprintf(file_out_p, "  PQclear(res);\n");
  fprintf(file_out_p, "}\n\n");

  // UPDATE function
  fprintf(file_out_p, "void Update%s(PGconn* conn, %s u, const char* where_clause)\n{\n", table_name, table_name);
  fprintf(file_out_p, "  char query[%i];\n", QUERY_BUFFER);

  // Emit code to write query
  fprintf(file_out_p, "  snprintf(query, sizeof(query),\n");
  fprintf(file_out_p, "    \"UPDATE %s \"\n    \"SET %s WHERE %%s;\",\n", table_name, set_clause);
  fprintf(file_out_p, "    %s, where_clause);\n", update_args);

  fprintf(file_out_p, "  PGresult* res = PQexec(conn, query);\n");
  fprintf(file_out_p, "  PQclear(res);\n");
  fprintf(file_out_p, "}\n\n");

  // DELETE stub
  fprintf(file_out_p, "void Delete%s(PGconn* conn, const char* where_clause)\n{\n", table_name);
  fprintf(file_out_p, "  char query[%i];\n", QUERY_BUFFER);
  fprintf(file_out_p, "  snprintf(query, sizeof(query),\n");
  fprintf(file_out_p, "    \"DELETE FROM %s WHERE %%s;\",\n", table_name);
  fprintf(file_out_p, "    where_clause);\n");
  fprintf(file_out_p, "  PGresult* res = PQexec(conn, query);\n");
  fprintf(file_out_p, "  PQclear(res);\n");
  fprintf(file_out_p, "}\n");

  fclose(file_out_p);
} 

void CreateCRUDFilesFromSQL(
  const char *sql_file_name
)
{
  FILE *fp; 
  char table_name[TABLE_LEN];
  char line[LINE_LEN];
  Column columns[MAX_COLUNM_NUM];
  int curr_column_num = 0;
  char *curr;
  int8_t is_in_table = 0;

  fp = fopen(sql_file_name, "r");

  while(fgets(line, LINE_LEN, fp))
  {
   if (IS_CREATE_TABLE(line))
   { 
     GetWord(line+CREATE_TABLE, table_name);
     is_in_table = 1;
     continue;
   }

   if (is_in_table)
   {
     char *skipped = SkipSpaces(line);
     if (*skipped=='\n' || *skipped==')') continue;
     int length = GetWord(skipped, columns[curr_column_num].name);
     if (strncmp(columns[curr_column_num].name, "--", 2)==0) continue;
     GetWord(SkipSpaces(skipped+length), columns[curr_column_num].type);
     curr_column_num++;
   }
  }
  CreateCRUDFiles(table_name, columns, curr_column_num);
}

void SetModelHeader(FILE *models_header)
{
  fprintf(models_header, "#ifndef MODELS_H\n#define MODELS_H\n\n");
}

pid_t* PIDStatus(
  const char *model_dir,
  const struct dirent *entry,
  FILE *models_header
)
{
   char full_path[512];
   snprintf(full_path, sizeof(full_path), "%s/%s", model_dir, entry->d_name);
   pid_t pid = fork();

	 switch (pid)
   {
	   case -1:
	       perror("fork");
	       exit(EXIT_FAILURE);
	   case 0:
	       puts("Child exiting.");
         char table_name[TABLE_LEN];
         printf("%s: legnth: %d\n", entry->d_name, strcspn(entry->d_name, "."));
         size_t len_name = strcspn(entry->d_name, ".");
         strncpy(table_name, entry->d_name, len_name);
         table_name[len_name] = '\0';
         printf("%s\n", table_name);
         fprintf(models_header, "#include \"model_%s.h\"\n", table_name);
	       exit(3);
	   default:
	       printf("Child is PID %jd\n", (int) pid);
         CreateCRUDFilesFromSQL(full_path);
	       puts("Parent exiting.");
	 }
}

int main()
{
  FILE *models_header;
  const char *model_dir = "/home/mrjunejune/project/pog_pool/models";
  DIR *dir = opendir(model_dir);
  struct dirent *entry;
  models_header = fopen("/home/mrjunejune/project/pog_pool/lib/models.h", "w");
  SetModelHeader(models_header);
  fclose(models_header);

  models_header = fopen("/home/mrjunejune/project/pog_pool/lib/models.h", "a");

  DIR *dir_copy = dir;

  while ((entry = readdir(dir)) != NULL)
  {
    // ignore vim
    if (entry->d_type == DT_REG && strstr(entry->d_name, ".swp"))
    {
      continue;
    }


    if (entry->d_type == DT_REG && strstr(entry->d_name, ".sql"))
    {
      PIDStatus(model_dir, entry, models_header);
    }
  }

  while (wait(NULL) > 0)
  {
    continue;
  }

  // close the parent headers.
  fprintf(models_header, "\n#endif // MODELS_H\n");
  fclose(models_header);
  closedir(dir);
  return 0;
}

