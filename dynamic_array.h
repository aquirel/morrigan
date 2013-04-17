// dynamic_array.h - dynamic arrays support.

#pragma once
#ifndef __DYNAMIC_ARRAY_H__
#define __DYNAMIC_ARRAY_H__

#pragma message("__DYNAMIC_ARRAY_H__")

#include <stdlib.h>
#include <stdbool.h>
#include <threads.h>

#include "morrigan.h"

#pragma pack(push, 4)

typedef struct DynamicArray
{
    size_t element_size;
    size_t element_count;
    size_t array_capacity;
    mtx_t guard_mutex;
    char *data;
} DynamicArray;

#pragma pack(pop)

DynamicArray *dynamic_array_create(size_t element_size, size_t element_count);
#define DYNAMIC_ARRAY_CREATE(element_type, element_count) dynamic_array_create(sizeof(element_type), (element_count))
void dynamic_array_destroy(DynamicArray *a);
void *dynamic_array_get(const DynamicArray *a, size_t i);
#define DYNAMIC_ARRAY_GET(element_type, a, i) ((element_type) dynamic_array_get((a), (i)))
const void *dynamic_array_set(DynamicArray *a, size_t i, const void *data);
bool dynamic_array_push(DynamicArray *a, void *data);
void *dynamic_array_pop(DynamicArray *a);
#define DYNAMIC_ARRAY_POP(element_type, a) ((element_type) dynamic_array_pop(a))
void dynamic_array_delete_at(DynamicArray *a, size_t i);
size_t dynamic_array_count(DynamicArray *a);

void dynamic_array_lock(DynamicArray *a);
void dynamic_array_unlock(DynamicArray *a);

#endif /* __DYNAMIC_ARRAY_H__ */
