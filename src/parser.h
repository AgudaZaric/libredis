#ifndef __PARSER_H
#define __PARSER_H

#include "reply.h"

typedef struct _ReplyParser ReplyParser;

typedef enum _ReplyParserResult
{
    RPR_ERROR = 0,
    RPR_MORE = 1,
    RPR_REPLY = 2
} ReplyParserResult;

ReplyParser *ReplyParser_new();
int ReplyParser_reset(ReplyParser *rp);
int ReplyParser_free(ReplyParser *rp);

ReplyParserResult ReplyParser_execute(ReplyParser *rp, Byte *buffer, size_t len, Reply **reply);

#endif
