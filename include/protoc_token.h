#ifndef PROTOC_TOK_H
#define PROTOC_TOK_H

#include <protoc.h>

typedef enum
{
	PROTOC_TOK_INVALID = -1,
	PROTOC_TOK_KEYWORD_MESSAGE = 0,
	PROTOC_TOK_KEYWORD_REQUIRED,
	PROTOC_TOK_KEYWORD_OPTIONAL,
	PROTOC_TOK_KEYWORD_REPEATED,
	PROTOC_TOK_KEYWORD_INT32,
	PROTOC_TOK_KEYWORD_UINT32,
	PROTOC_TOK_KEYWORD_BOOL,
	PROTOC_TOK_KEYWORD_STRING,
	PROTOC_TOK_KEYWORD_FIXED32,
	PROTOC_TOK_KEYWORD_FLOAT,
	PROTOC_TOK_LITERAL,
	PROTOC_TOK_NUMBER,
	PROTOC_TOK_CURLY_OPEN,
	PROTOC_TOK_CURLY_CLOSE,
	PROTOC_TOK_EQUALS,
	PROTOC_TOK_SEMICOLON,
	PROTOC_TOK_LINECOMMENT,
	PROTOC_TOK_BLOCKCOMMENT,
	PROTOC_TOK_WHITESPACE,
	PROTOC_SPECIAL_RANGE,
	PROTOC_SPECIAL_END
} protoc_token_type;

typedef struct
{
	char *token;
	protoc_token_type type;
	int line;
	int col;
	const char *description;
} protoc_token;

protoc_error protoc_tokenize(
	const char *string, 
	protoc_token **tokens, 
	size_t *count,
	const char *description,
	protoc_io *io);

#endif