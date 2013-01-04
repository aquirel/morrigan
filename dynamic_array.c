// dynamic_array.c - dynamic arrays support.

#include <assert.h>

#include "dynamic_array.h"
#include "debug.h"

DynamicArray *dynamic_array_create(size_t element_size, size_t array_capacity)
{
    assert(element_size > 0 && "Bad element_size.");
    assert(array_capacity > 0 && "Bad array_capacity.");

    DynamicArray *a = (DynamicArray *) calloc(1, sizeof(DynamicArray));
    check_mem(a);

    a->data = (char *) calloc(array_capacity, element_size);
    check_mem(a->data);

    a->element_size = element_size;
    a->element_count = 0;
    a->array_capacity = array_capacity;

    return a;

    error:
    if (a && a->data)
    {
        free(a->data);
    }
    if (a)
    {
        free(a);
    }
    return NULL;
}

void dynamic_array_destroy(DynamicArray *a)
{
    assert(a && "Nothing to destroy.");
    assert(a->data && "Nothing to destroy.");

    free(a->data);
    free(a);
}

static inline void __dynamic_array_assert(const DynamicArray *a);

void *dynamic_array_get(const DynamicArray *a, size_t i)
{
    __dynamic_array_assert(a);
    assert(i >= 0 && i < a->array_capacity && "Index out of range.");
    return &a->data[i * a->element_size];
}

const void *dynamic_array_set(DynamicArray *a, size_t i, const void *data)
{
    __dynamic_array_assert(a);
    assert(i >= 0 && i < a->array_capacity && "Index out of range.");
    memcpy(a->data + i * a->element_size, data, a->element_size);
    return data;
}

bool dynamic_array_push(DynamicArray *a, void *data)
{
    __dynamic_array_assert(a);

    if (a->element_count < a->array_capacity)
    {
        dynamic_array_set(a, a->element_count++, data);
        return true;
    }

    char *new_data = (char *) realloc(a->data, (1 + a->element_count) * a->element_size);
    check_mem(new_data);

    a->data = new_data;
    a->array_capacity++;
    dynamic_array_set(a, a->element_count++, data);
    return true;

    error:
    return false;
}

void *dynamic_array_pop(DynamicArray *a)
{
    __dynamic_array_assert(a);
    assert(a->element_count && "Nothing to pop.");

    return &a->data[--a->element_count * a->element_size];
}

void dynamic_array_delete_at(DynamicArray *a, size_t i)
{
    __dynamic_array_assert(a);
    assert(i >= 0 && i < a->element_count && "Index out of range.");
    a->element_count--;
    if (i == a->element_count)
    {
        memset(a->data + i * a->element_size, 0, a->element_size);
        return;
    }

    memmove(a->data + i * a->element_size, a->data + (i + 1) * a->element_size, a->element_count - i);
    memset(a->data + a->element_count * a->element_size, 0, a->element_size);
}

size_t dynamic_array_count(DynamicArray *a)
{
    __dynamic_array_assert(a);
    return a->element_count;
}

static inline void __dynamic_array_assert(const DynamicArray *a)
{
    assert(a && "Bad array.");
    assert(a->data && "Bad array.");
    assert(a->element_size && "Bad array.");
    assert(a->array_capacity && "Bad array.");
}

#if defined(DYNAMIC_ARRAY_TESTS)
#include <stdio.h>

#include "testhelp.h"

int main(void)
{
    DynamicArray *a = dynamic_array_create(sizeof(int), 5);
    test_cond("Check array creation.", a);

    int v = 0;
    test_cond("Push 0.", dynamic_array_push(a, &v));
    v = 1;
    test_cond("Push 1.", dynamic_array_push(a, &v));
    v = 2;
    test_cond("Push 2.", dynamic_array_push(a, &v));
    v = 3;
    test_cond("Push 3.", dynamic_array_push(a, &v));
    v = 4;
    test_cond("Push 4.", dynamic_array_push(a, &v));
    v = 5;
    test_cond("Push 5.", dynamic_array_push(a, &v));
    v = 6;
    test_cond("Push 6.", dynamic_array_push(a, &v));
    v = 7;
    test_cond("Push 7.", dynamic_array_push(a, &v));
    v = 8;
    test_cond("Push 8.", dynamic_array_push(a, &v));

    test_cond("Test get at 0.", 0 == *DYNAMIC_ARRAY_GET(int *, a, 0));
    test_cond("Test get at 8.", 8 == *DYNAMIC_ARRAY_GET(int *, a, 8));

    test_cond("Test pop 8.", 8 == *DYNAMIC_ARRAY_POP(int *, a));
    test_cond("Test pop 7.", 7 == *DYNAMIC_ARRAY_POP(int *, a));
    test_cond("Test pop 6.", 6 == *DYNAMIC_ARRAY_POP(int *, a));

    dynamic_array_delete_at(a, 1);
    test_cond("Test get at 1 after delete.", 2 == *DYNAMIC_ARRAY_GET(int *, a, 1));

    test_cond("Test get element count.", 5 == dynamic_array_count(a));

    dynamic_array_destroy(a);
    test_report();
    return EXIT_SUCCESS;
}

#endif
