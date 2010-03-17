#include "common.h"
#include "batch.h"
#include "list.h"
#include "command.h"
#include <stdio.h>

struct _Batch
{
	struct list_head write_queue; //commands queued for writing
	Buffer *write_buffer;
	Buffer *read_buffer;
};

Batch *Batch_new()
{
	Batch *batch = REDIS_ALLOC_T(Batch);
	batch->read_buffer = Buffer_new(DEFAULT_READ_BUFF_SIZE);
	batch->write_buffer = Buffer_new(DEFAULT_WRITE_BUFF_SIZE);
	INIT_LIST_HEAD(&batch->write_queue);
	return batch;
}

Reply *Reply_new()
{
	Reply *reply = REDIS_ALLOC_T(Reply);
	INIT_LIST_HEAD(&reply->children);
	return reply;
}

Command *Command_new()
{
	Command *command = REDIS_ALLOC_T(Command);
	return command;
}

int Batch_write_command(Batch *batch, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	size_t offset = Buffer_position(batch->write_buffer);
	Buffer_vprintf(batch->write_buffer, format, args);
	size_t len = Buffer_position(batch->write_buffer) - offset;
	va_end(args);

	Command *cmd = Command_new();
	cmd->batch = batch;
	cmd->write_buffer = batch->write_buffer;
	cmd->read_buffer = batch->read_buffer;
	cmd->offset = offset;
	cmd->len = len;

	list_add(&(cmd->list), &(batch->write_queue));

	//Connection_write_data(connection);
	return 0;
}

int Batch_execute(Batch *batch, Connection *connection)
{
	printf("batch execute\n");
	Connection_add_commands(connection, &batch->write_queue);
}


