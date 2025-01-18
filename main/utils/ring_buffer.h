#ifndef __RING_BUFFER_H
#define __RING_BUFFER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct _ring_buffer_t
{
    uint16_t* buffer;
    uint32_t head;
    uint32_t tail;
    uint32_t size;
    uint32_t count;
} ring_buffer_t;

bool ring_buffer_init(ring_buffer_t* rb, uint16_t* buffer, uint32_t size);
bool ring_buffer_is_empty(const ring_buffer_t* rb);
bool ring_buffer_is_full(const ring_buffer_t* rb);
uint32_t ring_buffer_get_count(const ring_buffer_t* rb);
uint32_t ring_buffer_get_free_space(const ring_buffer_t* rb);
bool ring_buffer_peek(const ring_buffer_t* rb, uint16_t* data, uint32_t offset);
bool ring_buffer_enqueue(ring_buffer_t* rb, uint16_t data);
bool ring_buffer_dequeue(ring_buffer_t* rb, uint16_t* data);

#endif /* __RING_BUFFER_H */