// array.h - dynamic arrays support.

#ifndef __ARRAY_H__
#define __ARRAY_H__

#include <stdlib.h>
#include <stdbool.h>

typedef struct DynamicArray
{
    size_t element_size;
    size_t element_count;
    size_t array_capacity;
    char *data;
} DynamicArray;

DynamicArray *dynamic_array_create(size_t element_size, size_t element_count);
#define DYNAMIC_ARRAY_CREATE(element_type, element_count) dynamic_array_create(sizeof(element_type), (element_count))
void dynamic_array_destroy(DynamicArray *a);
void *dynamic_array_get(const DynamicArray *a, size_t i);
#define DYNAMIC_ARRAY_GET(element_type, a, i) ((element_type) dynamic_array_get((a), (i)))
void *dynamic_array_set(DynamicArray *a, size_t i, void *data);
bool dynamic_array_push(DynamicArray *a, void *data);
void *dynamic_array_pop(DynamicArray *a);
#define DYNAMIC_ARRAY_POP(element_type, a) ((element_type) dynamic_array_pop(a))

#endif
