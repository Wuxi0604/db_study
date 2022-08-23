#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "db_study.h"


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

  if (bytes_read <= 0) {
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
prepare_statement(
  input_buffer_t*   input_buffer,
  statement_t*      statement
) 
{
  if (strncmp(input_buffer->buffer, "insert", 6) == 0) 
  {
    statement->type = STATEMENT_INSERT;
    return PREPARE_SUCCESS;
  }
  if (strcmp(input_buffer->buffer, "select") == 0)
   {
    statement->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }

  return PREPARE_UNRECOGNIZED_STATEMENT;
}

void 
execute_statement(
  statement_t* statement
) 
{
  switch (statement->type) 
  {
    case (STATEMENT_INSERT):
      printf("This is where we would do an insert.\n");
      break;
    case (STATEMENT_SELECT):
      printf("This is where we would do a select.\n");
      break;
  }
}


int main(
    int     argc, 
    char*   argv[]
) 
{
  input_buffer_t* input_buffer = new_input_buffer();
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
      case (PREPARE_UNRECOGNIZED_STATEMENT):
        printf("Unrecognized keyword at start of '%s'.\n", input_buffer->buffer);
        continue;
    }

    execute_statement(&statement);
    printf("Executed.\n");
  }
}