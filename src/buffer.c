#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>

#include "common.h"
#include "buffer.h"

struct _Buffer
{
	char *data;
	char *buff;
	size_t buff_size;
	int position;
	int limit;
	int capacity;
	int mark;
};

Buffer *Buffer_new(Alloc *alloc, size_t size)
{
	Buffer *buffer = (Buffer *)alloc->alloc(sizeof(Buffer));
	buffer->buff_size = size;
	buffer->buff = alloc->alloc(size);
	buffer->data = buffer->buff;
	buffer->position = 0;
	buffer->capacity = size;
	buffer->limit = buffer->capacity;
	buffer->mark = 0;
	return buffer;
}

int Buffer_dump(Buffer *buffer, int limit)
{
	int i, j;
	if(limit == -1) {
		limit = buffer->capacity;
	}
	printf("buffer cap: %d, limit: %d, pos: %d\n", buffer->capacity, buffer->limit, buffer->position);
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
}

int Buffer_remaining(Buffer *buffer)
{
    return (buffer->limit - buffer->position);
}

int Buffer_ensure_remaining(Buffer *buffer, int len)
{
	if(Buffer_remaining(buffer) < len) {
		//todo
	};
	return Buffer_remaining(buffer);
}


int Buffer_send(Buffer *buffer, int fd)
{
	printf("Buffer_send fd: %d, position: %d, limit: %d, remaining: %d\n", fd, buffer->position, buffer->limit, Buffer_remaining(buffer));
	size_t bytes_written = write(fd, buffer->data + buffer->position, Buffer_remaining(buffer));
	printf("Buffer_send fd: %d, bytes_written: %d\n", fd, bytes_written);
	if(bytes_written == -1) {
		printf("Buffer_send error fd: %d err: %d\n", fd, errno);
	}
	else {
		buffer->position += bytes_written;
	}
}

int Buffer_recv(Buffer *buffer, int fd, size_t len)
{
	printf("Buffer_recv fd: %d, position: %d, limit: %d, remaining: %d\n", fd, buffer->position, buffer->limit, Buffer_remaining(buffer));
	if(len == -1) {
		len = Buffer_remaining(buffer);
	}
	else {
		len = MIN(Buffer_ensure_remaining(buffer, len), len);
	}
	size_t bytes_read = read(fd, buffer->data + buffer->position, len);
	printf("Buffer_recv fd: %d, bytes_read: %d\n", fd, bytes_read);
	if(bytes_read == -1) {
		printf("Buffer_recv error fd: %d err: %d\n", fd, errno);
	}
	else {
		buffer->position += bytes_read;
	}
}

int Buffer_flip(Buffer *buffer)
{
	buffer->limit = buffer->position;
	buffer->position = 0;
}

/*
int Buffer_write_string(Buffer *buffer, const char *s, int len)
{
	Buffer_ensure_remaining(buffer, len);
	memcpy(buffer->data + buffer->position, s, len);
	buffer->position += len;
}
*/

int Buffer_printf(Buffer *buffer, const char *format, ...)
{
	va_list args;
	int remaining = buffer->limit - buffer->position;
	va_start(args, format);
	int written = vsnprintf(buffer->data + buffer->position, remaining, format, args);
	va_end(args);
	if(written > remaining) {
		Buffer_ensure_remaining(buffer, written);
		remaining = buffer->limit - buffer->position;
		va_start(args, format);
		written = vsnprintf(buffer->data + buffer->position, remaining, format, args);
		va_end(args);
		//TODO check written again
		printf("assert!");
	}
	buffer->position += written;
}
