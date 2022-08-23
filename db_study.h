#ifndef DB_STUDY_H
#define DB_STUDY_H
#include <stdlib.h>

typedef struct input_buffer_struct input_buffer_t;

struct input_buffer_struct
{
    char*   buffer;
    size_t  buffer_length;
    ssize_t input_length;
};


#endif