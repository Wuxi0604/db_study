#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "db_study.h"


#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

const uint32_t ID_SIZE        = size_of_attribute(row_t, id);
const uint32_t USERNAME_SIZE  = size_of_attribute(row_t, username);
const uint32_t EMAIL_SIZE     = size_of_attribute(row_t, email);
const uint32_t ID_OFFSET      = 0;

const uint32_t USERNAME_OFFSET  = ID_OFFSET + ID_SIZE;
const uint32_t EMAIL_OFFSET     = USERNAME_OFFSET + USERNAME_SIZE;
const uint32_t ROW_SIZE         = ID_SIZE + USERNAME_SIZE + EMAIL_SIZE;


const uint32_t PAGE_SIZE      = 4096;
const uint32_t ROWS_PER_PAGE  = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;

void* 
row_slot(
  table_t* table, 
  uint32_t row_num
) 
{
  uint32_t page_num   = row_num / ROWS_PER_PAGE;
  void* page          = table->pages[page_num];
  if (page == NULL) {
    // Allocate memory only when we try to access page
    page = table->pages[page_num] = malloc(PAGE_SIZE);
  }
  uint32_t row_offset   = row_num % ROWS_PER_PAGE;
  uint32_t byte_offset  = row_offset * ROW_SIZE;
  return page + byte_offset;
}

void 
serialize_row(
  row_t*  source, 
  void*   destination
) 
{
  memcpy(destination + ID_OFFSET, &(source->id), ID_SIZE);
  memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
  memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
}

void 
deserialize_row(
   void*  source,
   row_t* destination
) 
{
  memcpy(&(destination->id), source + ID_OFFSET, ID_SIZE);
  memcpy(&(destination->username), source + USERNAME_OFFSET, USERNAME_SIZE);
  memcpy(&(destination->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}

input_buffer_t* 
new_input_buffer()
 {
  input_buffer_t* input_buffer = malloc(sizeof(input_buffer_t));
  input_buffer->buffer          = NULL;
  input_buffer->buffer_length   = 0;
  input_buffer->input_length    = 0;

  return input_buffer;
}

void 
print_prompt() 
{ 
    printf("db > "); 
}

void 
read_input(
    input_buffer_t* input_buffer
)
 {
  /*
    ssize_t getline(char **lineptr, size_t *n, FILE *stream);
  */
  ssize_t bytes_read =
      getline(&(input_buffer->buffer), &(input_buffer->buffer_length), stdin);

  if (bytes_read <= 0) 
  {
    printf("Error reading input\n");
    exit(EXIT_FAILURE);
  }

  // Ignore trailing newline
  input_buffer->input_length            = bytes_read - 1;
  input_buffer->buffer[bytes_read - 1]  = 0;
}

void 
close_input_buffer(
    input_buffer_t* input_buffer
) 
{
    free(input_buffer->buffer);
    free(input_buffer);
}

meta_command_result_e 
do_meta_command(
  input_buffer_t* input_buffer
) 
{
  if (strcmp(input_buffer->buffer, ".exit") == 0) 
  {
    exit(EXIT_SUCCESS);
  } else {
    return META_COMMAND_UNRECONGNIZED_COMMAND;
  }
}

prepare_result_e 
prepare_insert(
  input_buffer_t* input_buffer, 
  statement_t*    statement
) 
{
  statement->type = STATEMENT_INSERT;

  char* keyword   = strtok(input_buffer->buffer, " ");
  char* id_string = strtok(NULL, " ");
  char* username  = strtok(NULL, " ");
  char* email     = strtok(NULL, " ");

  if (id_string == NULL || username == NULL || email == NULL) 
  {
    return PREPARE_SYNTAX_ERROR;
  }

  int id = atoi(id_string);
  if(id < 0)
  {
    return PREPARE_NEGATIVE_ID;
  }
  if (strlen(username) > COLUMN_USERNAME_SIZE) {
    return PREPARE_STRING_TOO_LONG;
  }
  if (strlen(email) > COLUMN_EMAIL_SIZE) 
  {
    return PREPARE_STRING_TOO_LONG;
  }

  statement->row_to_insert.id = id;
  strcpy(statement->row_to_insert.username, username);
  strcpy(statement->row_to_insert.email, email);

  return PREPARE_SUCCESS;
}


prepare_result_e 
prepare_statement(
  input_buffer_t*   input_buffer,
  statement_t*      statement
) 
{
  if (strncmp(input_buffer->buffer, "insert", 6) == 0) 
  {
    return prepare_insert(input_buffer, statement);
  }
  if (strcmp(input_buffer->buffer, "select") == 0)
   {
    statement->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }

  return PREPARE_UNRECOGNIZED_STATEMENT;
}

execute_result_e 
execute_insert(
  statement_t*  statement, 
  table_t*      table
) 
{
  if (table->num_rows >= TABLE_MAX_ROWS) 
  {
    return EXECUTE_TABLE_FULL;
  }

  row_t* row_to_insert = &(statement->row_to_insert);

  serialize_row(row_to_insert, row_slot(table, table->num_rows));
  table->num_rows += 1;

  return EXECUTE_SUCCESS;
}

void 
print_row(
  row_t* row
) 
{
  printf("(%d, %s, %s)\n", row->id, row->username, row->email);
}

execute_result_e
execute_select(
  statement_t*  statement, 
  table_t*      table
) 
{
  row_t row;
  for (uint32_t i = 0; i < table->num_rows; i++)
  {
    deserialize_row(row_slot(table, i), &row);
    print_row(&row);
  }
  return EXECUTE_SUCCESS;
}

execute_result_e 
execute_statement(
  statement_t*  statement,
  table_t*      table
) 
{
  switch (statement->type) 
  {
    case (STATEMENT_INSERT):
      return execute_insert(statement, table);
    case (STATEMENT_SELECT):
      return execute_select(statement, table);
  }
}

table_t* 
new_table() 
{
  table_t* table = (table_t*)malloc(sizeof(table_t));
  table->num_rows = 0;
  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) {
     table->pages[i] = NULL;
  }
  return table;
}

void 
free_table(
  table_t* table
) 
{
  for (int i = 0; table->pages[i]; i++) 
  {
	  free(table->pages[i]);
  }
    free(table);
}


int main(
    int     argc, 
    char*   argv[]
) 
{
  table_t* table                = new_table();
  input_buffer_t* input_buffer  = new_input_buffer();
  while (true) 
  {
    print_prompt();
    read_input(input_buffer);

    if(input_buffer->buffer[0] == '.')
    {
      switch(do_meta_command(input_buffer))
      {
        case (META_COMMAND_SUCCESS):
          continue;
        case (META_COMMAND_UNRECONGNIZED_COMMAND):
          printf("Unrecognized command '%s'\n", input_buffer->buffer);
          continue;
      }
    }
    statement_t statement;
    switch(prepare_statement(input_buffer, &statement))
    {
      case (PREPARE_SUCCESS):
        break;
      case (PREPARE_NEGATIVE_ID):
        printf("ID must be positive.\n"); 
        continue; 
      case (PREPARE_STRING_TOO_LONG):
        printf("String is too long.\n");
        continue;
      case (PREPARE_SYNTAX_ERROR):
        printf("Syntax error. Could not parse statement.\n");
        continue;
      case (PREPARE_UNRECOGNIZED_STATEMENT):
        printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
        continue;
    }

    switch (execute_statement(&statement, table))
    {
    case (EXECUTE_SUCCESS):
      printf("Executed.\n");
      break;
    case (EXECUTE_TABLE_FULL):
      printf("Error: Table full.\n");
      break;
    }
  }
}