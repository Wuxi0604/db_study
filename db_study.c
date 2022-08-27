#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
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

void
cursor_advance(
  cursor_t*   cursor
)
{
  cursor->row_num += 1;
  if(cursor->row_num >= cursor->table->num_rows)
  {
    cursor->end_of_table = true;
  }
}

cursor_t*
table_start(
  table_t*    table
)
{
  cursor_t* cursor      = malloc(sizeof(cursor_t));
  cursor->table         = table;
  cursor->row_num       = 0;
  cursor->end_of_table  = (table->num_rows == 0);

  return cursor;
}

cursor_t*
table_end(
  table_t* table
)
{
  cursor_t* cursor  = malloc(sizeof(cursor_t));
  cursor->table     = table;
  cursor->row_num   = table->num_rows;

  cursor->end_of_table = true;
  return cursor;
}

void 
pager_flush(
  pager_t* pager, 
  uint32_t page_num, 
  uint32_t size
)
{
  if (pager->pages[page_num] == NULL) 
  {
    printf("Tried to flush null page\n");
    exit(EXIT_FAILURE);
  }

  off_t offset = lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);

  if (offset == -1) {
    printf("Error seeking: %d\n", errno);
    exit(EXIT_FAILURE);
  }

  ssize_t bytes_written = write(pager->file_descriptor, pager->pages[page_num], size);

  if (bytes_written == -1) 
  {
    printf("Error writing: %d\n", errno);
    exit(EXIT_FAILURE);
  }
}

void 
db_close(
  table_t* table
) 

{
  pager_t* pager          = table->pager;
  uint32_t num_full_pages = table->num_rows / ROWS_PER_PAGE;

  for (uint32_t i = 0; i < num_full_pages; i++) 
  {
    if (pager->pages[i] == NULL) 
    {
      continue;
    }
    pager_flush(pager, i, PAGE_SIZE);
    free(pager->pages[i]);
    pager->pages[i] = NULL;
  }

  // There may be a partial page to write to the end of the file
  // This should not be needed after we switch to a B-tree
  uint32_t num_additional_rows = table->num_rows % ROWS_PER_PAGE;
  if (num_additional_rows > 0) 
  {
    uint32_t page_num = num_full_pages;
    if (pager->pages[page_num] != NULL) 
    {
      pager_flush(pager, page_num, num_additional_rows * ROW_SIZE);
      free(pager->pages[page_num]);
      pager->pages[page_num] = NULL;
    }
  }

  int result = close(pager->file_descriptor);
  if (result == -1) 
  {
    printf("Error closing db file.\n");
    exit(EXIT_FAILURE);
  }
  for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) 
  {
    void* page = pager->pages[i];
    if (page) 
    {
      free(page);
      pager->pages[i] = NULL;
    }
  }
  free(pager);
  free(table);
}

void*
get_page(
  pager_t*    pager,
  uint32_t    page_num
)
{
  if(page_num > TABLE_MAX_PAGES)
  {
    printf("Tried to fetch page number out of bounds. %d > %d\n", page_num, TABLE_MAX_PAGES);
    exit(EXIT_FAILURE);
  }
  if(pager->pages[page_num] == NULL)
  {
    // Cache miss. Allocate memory and load from file.
    void*     page      = malloc(PAGE_SIZE);
    uint32_t  num_pages = pager->file_length / PAGE_SIZE;
    // We might save a partial page at the end of the file
    if (pager->file_length % PAGE_SIZE) 
    {
      num_pages += 1;
    }

    if (page_num <= num_pages) 
    {
      lseek(pager->file_descriptor, page_num * PAGE_SIZE, SEEK_SET);
      ssize_t bytes_read = read(pager->file_descriptor, page, PAGE_SIZE);
      if (bytes_read == -1) 
      {
        printf("Error reading file: %d\n", errno);
        exit(EXIT_FAILURE);
      }
    }

    pager->pages[page_num] = page;
  }
  return pager->pages[page_num];
}

void* 
cursor_value(
  cursor_t*   cursor
) 
{
  uint32_t row_num      = cursor->row_num;
  uint32_t page_num     = row_num / ROWS_PER_PAGE;
  void*    page         = get_page(cursor->table->pager, page_num);
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
  // memcpy(destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
  // memcpy(destination + EMAIL_OFFSET, &(source->email), EMAIL_SIZE);
  /*
  If we wanted to ensure that all bytes are initialized, 
  it would suffice to use strncpy instead of memcpy
   while copying the username and email fields of rows 
   in serialize_row
  */
  
  strncpy(destination + USERNAME_OFFSET, source->username, USERNAME_SIZE);
  strncpy(destination + EMAIL_OFFSET, source->email, EMAIL_SIZE);
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
  input_buffer_t* input_buffer,
  table_t*        table
) 
{
  if (strcmp(input_buffer->buffer, ".exit") == 0) 
  {
    db_close(table);
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

  row_t*    row_to_insert = &(statement->row_to_insert);
  cursor_t* cursor        = table_end(table);

  serialize_row(row_to_insert, cursor_value(cursor));
  table->num_rows += 1;
  free(cursor);
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
  row_t     row;
  cursor_t* cursor = table_start(table);
  while (!(cursor->end_of_table))
  {
    deserialize_row(cursor_value(cursor), &row);
    print_row(&row);
    cursor_advance(cursor);
  }

  free(cursor);

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

pager_t*
pager_open(
  const char* filename
)
{
   int fd = open(filename,
                O_RDWR |      // Read/Write mode
                    O_CREAT,  // Create file if it does not exist
                S_IWUSR |     // User write permission
                    S_IRUSR   // User read permission
                );

    if (fd == -1)
    {
      printf("Unable to open file.\n");
      exit(EXIT_FAILURE);
    }

    off_t     file_length = lseek(fd, 0, SEEK_END);
    pager_t*  pager       = malloc(sizeof(pager_t));
    pager->file_descriptor = fd;
    pager->file_length     = file_length; 
    for (uint32_t i = 0; i < TABLE_MAX_PAGES; i++) 
    {
      pager->pages[i] = NULL;
    }

    return pager;
}

table_t* 
db_open(
  const char* filename
) 
{
  pager_t* pager     = pager_open(filename);
  uint32_t num_rows  = pager->file_length / ROW_SIZE;
  table_t* table     = (table_t*)malloc(sizeof(table_t));
  table->pager       = pager;
  table->num_rows    = num_rows;
  return table;
}

// void 
// free_table(
//   table_t* table
// ) 
// {
//   for (int i = 0; table->pages[i]; i++) 
//   {
// 	  free(table->pages[i]);
//   }
//     free(table);
// }


int main(
    int     argc, 
    char*   argv[]
) 
{
  if(argc < 2)
  {
    printf("Must supply a database filename.\n");
    exit(EXIT_FAILURE);
  }
  char*     filename  = argv[1];
  table_t*  table     = db_open(filename);
  input_buffer_t* input_buffer  = new_input_buffer();
  while (true) 
  {
    print_prompt();
    read_input(input_buffer);

    if(input_buffer->buffer[0] == '.')
    {
      switch(do_meta_command(input_buffer, table))
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