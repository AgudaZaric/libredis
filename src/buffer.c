#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "common.h"
#include "buffer.h"
#include "list.h"

struct _Buffer
{
	Byte *data; //the current data (at first points to buff, but by enlargement might point to some enlarged buffer
	Byte *buff; //the original buffer as allocated when buffer created
	size_t buff_size; //original buffer size
	size_t position;
	int limit;
	int capacity; //current capacity
};

Buffer *Buffer_new(size_t size)
{
	Buffer *buffer = Alloc_alloc_T(Buffer);
	buffer->buff_size = size;
	buffer->buff = Alloc_alloc(size);
	buffer->data = buffer->buff;
	buffer->position = 0;
	buffer->capacity = size;
	buffer->limit = buffer->capacity;
#ifndef NDEBUG
	Buffer_fill(buffer, (Byte)0xEA);
#endif
	return buffer;
}

void Buffer_fill(Buffer *buffer, Byte b)
{
	for(int i = 0; i < buffer->capacity; i++) {
		buffer->buff[i] = b;
	}
}

void Buffer_clear(Buffer *buffer)
{
	if(buffer->capacity > buffer->buff_size) {
		DEBUG(("Clearing enlarged buffer\n"));
		Alloc_free(buffer->data, buffer->capacity);
	}
	buffer->data = buffer->buff;
	buffer->position = 0;
	buffer->limit = buffer->buff_size;
	buffer->capacity = buffer->buff_size;
}

int Buffer_free(Buffer *buffer)
{
	Alloc_free(buffer->buff, buffer->capacity);
	DEBUG(("dealloc Buffer\n"));
	Alloc_free_T(buffer, Buffer);
	return 0;
}


Byte *Buffer_data(Buffer *buffer)
{
	return buffer->data;
}

int Buffer_dump(Buffer *buffer, int limit)
{
	int i, j;
	if(limit == -1) {
		limit = buffer->capacity;
	}
	printf("buffer cap: %d, limit: %d, pos: %d\n", (int)buffer->capacity, (int)buffer->limit, (int)buffer->position);
	for(i = 0; i < limit; i+=16) {
		for(j = 0; j < 16; j++) {
			printf("%02X ", ((unsigned char *)buffer->data)[i + j]);
		}
		for(j = 0; j < 16; j++) {
			int c = ((unsigned char *)buffer->data)[i + j];
			if(isprint(c)) {
				printf("%c", c);
			}
			else {
				printf(".");
			}
		}
		printf("\n");
	}
	return 0;
}

int Buffer_remaining(Buffer *buffer)
{
    return (buffer->limit - buffer->position);
}

size_t Buffer_position(Buffer *buffer)
{
	return buffer->position;
}

int Buffer_set_position(Buffer *buffer, size_t position)
{
	buffer->position = position;
	return 0;
}

int Buffer_set_limit(Buffer *buffer, int limit)
{
	buffer->limit = limit;
	return 0;
}

int Buffer_ensure_remaining(Buffer *buffer, int min_remaining)
{
	assert(buffer->limit == buffer->capacity);
	while(Buffer_remaining(buffer) < min_remaining) {
		buffer->limit *= 2;
	}
	assert(buffer->limit >= buffer->capacity);
	if(buffer->limit > buffer->capacity) {
		if(buffer->capacity == buffer->buff_size) {
			DEBUG(("growing buffer first time, new cap: %d, old cap: %d\n", buffer->limit, buffer->capacity));
			buffer->data = Alloc_alloc(buffer->limit);
			memcpy(buffer->data, buffer->buff, buffer->capacity);
		}
		else {
			DEBUG(("growing buffer second or more time, new cap: %d, old cap: %d\n", buffer->limit, buffer->capacity));
			buffer->data = Alloc_realloc(buffer->data, buffer->limit, buffer->capacity);
		}
		buffer->capacity = buffer->limit;
	}
	assert(buffer->limit == buffer->capacity);
	return Buffer_remaining(buffer);
}

size_t Buffer_send(Buffer *buffer, int fd)
{
	DEBUG(("Buffer_send fd: %d, position: %d, limit: %d, remaining: %d\n", fd, buffer->position, buffer->limit, Buffer_remaining(buffer)));
	size_t bytes_written = write(fd, buffer->data + buffer->position, Buffer_remaining(buffer));
	DEBUG(("Buffer_send fd: %d, bytes_written: %d\n", fd, bytes_written));
	if(bytes_written != -1) {
		buffer->position += bytes_written;
	}
	return bytes_written;
}

size_t Buffer_recv(Buffer *buffer, int fd)
{
	Buffer_ensure_remaining(buffer, (buffer->capacity * 256) / 2048); //make sure we have still 1/8 remaining
	DEBUG(("Buffer_recv fd: %d, position: %d, limit: %d, remaining: %d\n", fd, buffer->position, buffer->limit, Buffer_remaining(buffer)));
	size_t bytes_read = read(fd, buffer->data + buffer->position, Buffer_remaining(buffer));
	DEBUG(("Buffer_recv fd: %d, bytes_read: %d\n", fd, bytes_read));
	if(bytes_read != -1) {
		buffer->position += bytes_read;
	}
	return bytes_read;
}

int Buffer_flip(Buffer *buffer)
{
	buffer->limit = buffer->position;
	buffer->position = 0;
	return 0;
}

int Buffer_write(Buffer *buffer, const char *data, size_t len)
{
	Buffer_ensure_remaining(buffer, len);
	memcpy(buffer->data + buffer->position, data, len);
	buffer->position += len;
	return 0;
}

