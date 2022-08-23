#ifndef DB_STUDY_H
#define DB_STUDY_H
#include <stdlib.h>

typedef struct input_buffer_struct  input_buffer_t;
typedef struct statement_struct     statement_t;

typedef enum meta_command_result_enum   meta_command_result_e;
typedef enum prepare_result_enum        prepare_result_e;
typedef enum statement_type_enum        statement_type_e;

enum prepare_result_enum
{
    PREPARE_SUCCESS, 
    PREPARE_UNRECOGNIZED_STATEMENT
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

struct statement_struct
{
    statement_type_e type;
};

struct input_buffer_struct
{
    char*   buffer;
    size_t  buffer_length;
    ssize_t input_length;
};


#endif