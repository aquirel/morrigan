// ring_buffer.c - ring buffer implementation.

#include <assert.h>

#include "ring_buffer.h"
#include "dynamic_array.h"
#include "debug.h"

static inline void __ring_buffer_assert(const RingBuffer *rb);

RingBuffer *ring_buffer_create(size_t element_size, size_t buffer_capacity)
{
    assert(element_size > 0 && "Bad element size.");
    assert(buffer_capacity > 0 && "Bad buffer capacity.");

    RingBuffer *rb = (RingBuffer *) calloc(1, sizeof(RingBuffer));
    check_mem(rb);

    rb->array = dynamic_array_create(element_size, buffer_capacity);
    check_mem(rb->array);

    check(thrd_success == cnd_init(&rb->buffer_not_empty), "Failed to initialize wait condition variable.", "");
    check(thrd_success == mtx_init(&rb->buffer_not_empty_mutex, mtx_plain), "Failed to initialize wait condition mutex.", "");
    assert(thrd_success == mtx_lock(&rb->buffer_not_empty_mutex) && "Failed to lock wait condition mutex.");

    rb->read_index = 0;
    rb->write_index = 0;

    return rb;

    error:
    if (rb && rb->array)
    {
        dynamic_array_destroy(rb->array);
    }
    if (rb)
    {
        cnd_destroy(&rb->buffer_not_empty);
        mtx_destroy(&rb->buffer_not_empty_mutex);
        free(rb);
    }
    return NULL;
}

void ring_buffer_destroy(RingBuffer *rb)
{
    assert(rb && "Nothing to destroy.");
    assert(rb->array && "Nothing to destroy.");

    dynamic_array_destroy(rb->array);
    cnd_destroy(&rb->buffer_not_empty);
    mtx_destroy(&rb->buffer_not_empty_mutex);
    free(rb);
}

bool ring_buffer_write(RingBuffer *rb, const void *data)
{
    __ring_buffer_assert(rb);

    if (ring_buffer_is_full(rb))
    {
        return false;
    }

    dynamic_array_set(rb->array, rb->write_index, data);
    rb->write_index = (rb->write_index + 1) % rb->array->array_capacity;
    rb->array->element_count++;
    assert(thrd_success == cnd_signal(&rb->buffer_not_empty) && "Failed to signal condition variable.");
    return true;
}

void *ring_buffer_read(RingBuffer *rb)
{
    __ring_buffer_assert(rb);

    if (ring_buffer_is_empty(rb))
    {
        return NULL;
    }

    char *result = dynamic_array_get(rb->array, rb->read_index);
    rb->read_index = (rb->read_index + 1) % rb->array->array_capacity;
    rb->array->element_count--;
    return result;
}

inline size_t ring_buffer_items_count(RingBuffer *rb)
{
    __ring_buffer_assert(rb);
    return rb->array->element_count;
}

inline bool ring_buffer_is_full(RingBuffer *rb)
{
    return ring_buffer_items_count(rb) == rb->array->array_capacity;
}

bool ring_buffer_is_empty(RingBuffer *rb)
{
    return 0 == ring_buffer_items_count(rb);
}

void ring_buffer_wait_not_empty(RingBuffer *rb)
{
    __ring_buffer_assert(rb);
    cnd_wait(&rb->buffer_not_empty, &rb->buffer_not_empty_mutex);
}

static inline void __ring_buffer_assert(const RingBuffer *rb)
{
    assert(rb && "Bad ring buffer.");
    assert(rb->array && "Bad ring buffer.");
}

#if defined(RING_BUFFER_TESTS)
#include <stdio.h>

#include "testhelp.h"

int main(void)
{
    RingBuffer *rb = ring_buffer_create(sizeof(int), 4);
    test_cond("Check buffer creation.", rb);

    int v = 0;
    test_cond("Write 0.", ring_buffer_write(rb, &v));
    v = 1;
    test_cond("Write 1.", ring_buffer_write(rb, &v));
    v = 2;
    test_cond("Write 2.", ring_buffer_write(rb, &v));
    v = 3;
    test_cond("Write 3.", ring_buffer_write(rb, &v));
    v = 4;
    test_cond("Write 4.", !ring_buffer_write(rb, &v));

    test_cond("Read 0.", 0 == *RING_BUFFER_READ(int *, rb));
    test_cond("Read 1.", 1 == *RING_BUFFER_READ(int *, rb));
    test_cond("Read 2.", 2 == *RING_BUFFER_READ(int *, rb));
    test_cond("Read 3.", 3 == *RING_BUFFER_READ(int *, rb));
    test_cond("Read 4.", NULL == ring_buffer_read(rb));

    ring_buffer_destroy(rb);
    test_report();
    return EXIT_SUCCESS;
}

#endif
