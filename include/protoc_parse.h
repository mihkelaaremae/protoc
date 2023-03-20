#ifndef PROTOC_PARSE_H
#define PROTOC_PARSE_H

#include <protoc_token.h>

protoc_error protoc_parse(
	protoc_message **messages,
	size_t *message_count,
	protoc_token *tokens, 
	size_t token_count,
	protoc_io *io);

#endif