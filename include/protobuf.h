#ifndef PROTOBUF_H
#define PROTOBUF_H

typedef enum
{
	PROTOBUF_SUCCESS = 0,
	PROTOBUF_SMALL_BUFFER = 1,
	PROTOBUF_UNKNOWN_WIRE = 2,
	PROTOBUF_UNEXPECTED_WIRE = 3,
} protobuf_error;

#endif