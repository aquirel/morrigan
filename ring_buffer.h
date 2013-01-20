// ring_buffer.h - ring buffer implementation.

#pragma once
#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

#include <threads.h>

#include "dynamic_array.h"

typedef struct RingBuffer
{
    DynamicArray *array;
    size_t read_index; // Index of first available item to read.
    size_t write_index; // Index of first free position to write.
    cnd_t buffer_not_empty;
    mtx_t buffer_not_empty_mutex;
} RingBuffer;

RingBuffer *ring_buffer_create(size_t element_size, size_t buffer_capacity);
#define RING_BUFFER_CREATE(element_type, buffer_capacity) ring_buffer_create(sizeof(element_type), (buffer_capacity))
void ring_buffer_destroy(RingBuffer *rb);
bool ring_buffer_write(RingBuffer *rb, const void *data);
void *ring_buffer_read(RingBuffer *rb);
#define RING_BUFFER_READ(element_type, rb) ((element_type) ring_buffer_read(rb))
inline size_t ring_buffer_items_count(RingBuffer *rb);
inline bool ring_buffer_is_full(RingBuffer *rb);
bool ring_buffer_is_empty(RingBuffer *rb);
void ring_buffer_wait_not_empty(RingBuffer *rb);

#endif /* __RING_BUFFER_H__ */
