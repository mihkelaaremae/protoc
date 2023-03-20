#ifndef PROTOC_H
#define PROTOC_H

#include <stdint.h>
#include <stdio.h>

extern const char *protoc_error_interpret[];

typedef enum
{
	PROTOC_SUCCESS,
	PROTOC_BAD_IN,
	PROTOC_BAD_OUT,
	PROTOC_EMPTY,
	PROTOC_SYNTAX_ERROR,
	PROTOC_TAG_CONFLICT
} protoc_error;

typedef enum
{
	PROTOC_WIRE_VARINT,
	PROTOC_WIRE_I64,
	PROTOC_WIRE_LEN,
	PROTOC_WIRE_SGROUP,
	PROTOC_WIRE_EGROUP,
	PROTOC_WIRE_I32
} protoc_wire_type;

typedef enum
{
	PROTOC_INT32,
	PROTOC_UINT32,
	PROTOC_BOOL,
	PROTOC_STRING,
	PROTOC_FIXED32,
	PROTOC_FLOAT32
} protoc_message_type;

typedef enum
{
	PROTOC_REQUIRED,
	PROTOC_OPTIONAL,
	PROTOC_REPEATED,
} protoc_message_qualifier;

typedef struct
{
	char *name;
	protoc_message_qualifier qual;
	protoc_message_type type;
	uint32_t index;
} protoc_field;

typedef struct
{
	char *name;
	protoc_field *fields;
	size_t field_count;
} protoc_message;

typedef struct
{
	struct
	{
		//Only one of these three should be set
		const char **filenames;
		FILE **files;
		protoc_message *messages;

		size_t count;				//Count for one of the arrays up above
		const char *header_name;	//Header name in the source
		const char *out_prefix;		//This or header_out along with source_out should be set
		FILE *header_out;			//Header code output
		FILE *source_out;			//Source code output
		int merge_messages;			//source_out is ignored
	} in;
	struct
	{
		//These are only set if PROTOC_SYNTAX_ERROR is returned
		const char *relevant_file;
		int linenum;
		int colnum;
	} out;
} protoc_io;

protoc_error protoc_compile(protoc_io *io);

#endif