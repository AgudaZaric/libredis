#ifndef __COMMAND_H
#define __COMMAND_H

#include "common.h"
#include "list.h"

Command *Command_new();
int Command_free(Command *command);
Command *Command_list_last(struct list_head *head);
Command *Command_list_pop(struct list_head *head);

Reply *Command_reply(Command *cmd);
Batch *Command_batch(Command *cmd);

#endif
