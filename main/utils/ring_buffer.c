#include "ring_buffer.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool ring_buffer_init(ring_buffer_t* rb, uint16_t* buffer, uint32_t size)
{
    if (rb == NULL || buffer == NULL)
    {
        return false;
    }

    if (size == 0 || rb->buffer != NULL)
    {
        return false;
    }

    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
    rb->size = size;
    rb->buffer = buffer;
    return true;
}

bool ring_buffer_is_empty(const ring_buffer_t* rb)
{
    if (rb == NULL)
    {
        return false;
    }

    return rb->count == 0;
}

bool ring_buffer_is_full(const ring_buffer_t* rb)
{
    if (rb == NULL)
    {
        return false;
    }

    return rb->count == rb->size;
}

uint32_t ring_buffer_get_count(const ring_buffer_t* rb)
{
    if (rb == NULL)
    {
        return false;
    }

    return rb->count;
}

uint32_t ring_buffer_get_free_space(const ring_buffer_t* rb)
{
    if (rb == NULL)
    {
        return false;
    }

    return rb->size - rb->count;
}

bool ring_buffer_peek(const ring_buffer_t* rb, uint16_t* data, uint32_t offset)
{
    if (rb == NULL || data == NULL)
    {
        return false;
    }

    if (rb->count == 0 || offset >= rb->count)
    {
        return false;
    }

    // Calculate the actual index in the circular buffer
    uint32_t index = (rb->tail + offset) % rb->size;

    // Retrieve the data
    *data = rb->buffer[index];
    return true; // Success
}

bool ring_buffer_enqueue(ring_buffer_t* rb, uint16_t data)
{
    if (rb == NULL)
    {
        return false;
    }

    if (ring_buffer_is_full(rb))
    {
        // Overwrite the oldest data
        rb->buffer[rb->head] = data;
        rb->head = (rb->head + 1) % rb->size; // Advance the head pointer
        rb->tail = (rb->tail + 1) % rb->size; // Advance the tail pointer
        // Do NOT increment `count` because it should stay equal to `size`
    }
    else
    {
        rb->buffer[rb->head] = data;
        rb->head = (rb->head + 1) % rb->size; // Advance the head pointer
        rb->count++;
    }
    return true; // Success
}

bool ring_buffer_dequeue(ring_buffer_t* rb, uint16_t* data)
{
    if (rb == NULL || data == NULL)
    {
        return false;
    }

    if (ring_buffer_is_empty(rb))
    {
        return false;
    }

    *data = rb->buffer[rb->tail];
    rb->tail = (rb->tail + 1) % rb->size;
    rb->count--;
    return true;
}