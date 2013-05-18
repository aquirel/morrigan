﻿// ring_buffer.h - ring buffer implementation.

#pragma once
#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__

//#pragma message("__RING_BUFFER_H__")

#include <threads.h>

#include "morrigan.h"
#include "dynamic_array.h"

#pragma pack(push, 4)

typedef struct RingBuffer
{
    DynamicArray *array;
    size_t read_index; // Index of first available item to read.
    size_t write_index; // Index of first free position to write.
    cnd_t buffer_not_empty;
    mtx_t buffer_not_empty_mutex;
} RingBuffer;

#pragma pack(pop)

RingBuffer *ring_buffer_create(size_t element_size, size_t buffer_capacity);
#define RING_BUFFER_CREATE(element_type, buffer_capacity) ring_buffer_create(sizeof(element_type), (buffer_capacity))
void ring_buffer_destroy(RingBuffer *rb);
bool ring_buffer_write(RingBuffer *rb, const void *data);
void *ring_buffer_read(RingBuffer *rb);
#define RING_BUFFER_READ(element_type, rb) ((element_type) ring_buffer_read(rb))
size_t ring_buffer_items_count(const RingBuffer *rb);
bool ring_buffer_is_full(const RingBuffer *rb);
bool ring_buffer_is_empty(const RingBuffer *rb);
void ring_buffer_wait_not_empty(RingBuffer *rb);

void ring_buffer_lock(RingBuffer *rb);
void ring_buffer_unlock(RingBuffer *rb);

#endif /* __RING_BUFFER_H__ */
