#ifndef DB_STUDY_H
#define DB_STUDY_H
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

typedef struct input_buffer_struct  input_buffer_t;
typedef struct statement_struct     statement_t;
typedef struct row_struct           row_t;
typedef struct table_struct         table_t;
typedef struct pager_struct         pager_t;
typedef struct cursor_struct        cursor_t;


typedef enum meta_command_result_enum   meta_command_result_e;
typedef enum prepare_result_enum        prepare_result_e;
typedef enum statement_type_enum        statement_type_e;
typedef enum execute_result_enum        execute_result_e;

#define COLUMN_USERNAME_SIZE    32
#define COLUMN_EMAIL_SIZE       255

#define TABLE_MAX_PAGES         100

struct cursor_struct
{
    table_t*        table;
    uint32_t        row_num;
    bool            end_of_table;// Indicates a position one past the last element
};

struct pager_struct
{
    int         file_descriptor;
    uint32_t    file_length;
    void*       pages[TABLE_MAX_PAGES];
};

struct table_struct{
  uint32_t  num_rows;
  pager_t*  pager;
//  void*     pages[TABLE_MAX_PAGES];
};

struct row_struct
{
    uint32_t    id;
    char        username[COLUMN_USERNAME_SIZE + 1];
    char        email[COLUMN_EMAIL_SIZE + 1];
};

enum prepare_result_enum
{
    PREPARE_SUCCESS, 
    PREPARE_SYNTAX_ERROR,
    PREPARE_UNRECOGNIZED_STATEMENT,
    PREPARE_STRING_TOO_LONG,
    PREPARE_NEGATIVE_ID
};

enum meta_command_result_enum
{
    META_COMMAND_SUCCESS,
    META_COMMAND_UNRECONGNIZED_COMMAND
};

enum statement_type_enum
{ 
    STATEMENT_INSERT, 
    STATEMENT_SELECT 
};

enum execute_result_enum{
  EXECUTE_SUCCESS,
  EXECUTE_DUPLICATE_KEY,
  EXECUTE_TABLE_FULL
};

struct statement_struct
{
    statement_type_e    type;
    row_t               row_to_insert;
};

struct input_buffer_struct
{
    char*   buffer;
    size_t  buffer_length;
    ssize_t input_length;
};


#endif