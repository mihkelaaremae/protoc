#include <protoc.h>
#include <stdlib.h>
#include <time.h>

static inline int protoc_get_msb64(uint64_t value)
{
	int count = 0;
	while (value >>= 1)
	{
	    count ++;
	}
	return count;
}
static inline int protoc_get_varint_size(uint64_t value)
{
	return protoc_get_msb64(value)/8 + 1;
}
static inline int protoc_get_tag_size(uint32_t index)
{
	return protoc_get_msb64(index << 3)/8 + 1;
}

void protoc_write_header_struct(
	protoc_message *m,
	FILE *out)
{
	const char *equivalent_type[] = 
	{
		[PROTOC_INT32] = "int32_t ",
		[PROTOC_UINT32] = "uint32_t ",
		[PROTOC_BOOL] = "int32_t ",
		[PROTOC_STRING] = "char *",
		[PROTOC_FIXED32] = "int32_t ",
		[PROTOC_FLOAT32] = "float ",
	};
	size_t i;
	protoc_field *f;
	fprintf(out, "typedef struct\n{\n");
	for (i = 0; i < m->field_count; i++)
	{
		f = m->fields + i;
		if (f->qual == PROTOC_OPTIONAL)
		{
			fprintf(out, "\t //%u\n\t%s%s;\n", f->index, equivalent_type[f->type], f->name);
			if (f->type == PROTOC_STRING)
			{
				fprintf(out, "\tsize_t %s_length;\n", f->name);
			}
			fprintf(out, "\tint %s_exists;\n", f->name);
		}
		else if (f->qual == PROTOC_REPEATED)
		{
			fprintf(out, "\t//%u\n\t%s*%s;\n", f->index, equivalent_type[f->type], f->name);
			if (f->type == PROTOC_STRING)
			{
				fprintf(out, "\tsize_t *%s_lengths;\n", f->name);
			}
			fprintf(out, "\tsize_t %s_count;\n", f->name);
		}
		else
		{
			fprintf(out, "\t//%u\n\t%s%s;\n", f->index, equivalent_type[f->type], f->name);
			if (f->type == PROTOC_STRING)
			{
				fprintf(out, "\tsize_t %s_length;\n", f->name);
			}
		}
	}
	fprintf(out, "} %s;\n", m->name);
}

void protoc_write_header_prologue(
	protoc_message *messages,
	size_t count,
	FILE *out)
{
	size_t i;
	int number;
	srand(time(NULL) + (uintptr_t)messages); 
	number = rand();
	fprintf(out, "#ifndef PROTOBUF_%d_H\n", number);
	fprintf(out, "#define PROTOBUF_%d_H\n", number);
	fputs("\n", out);
	fputs("#include <stdio.h>\n", out);
	fputs("#include <stdint.h>\n", out);
	fputs("#include <string.h>\n", out);
	fputs("\n", out);
	for (i = 0; i < count; i++)
	{
		protoc_write_header_struct(messages + i, out);
	}
	fputs("\n", out);
	for (i = 0; i < count; i++)
	{
		fprintf(out, "size_t protobuf_buffersize_%s(%s *message);\n\n", messages[i].name, messages[i].name);
		fprintf(out, "int protobuf_write_mem_%s(\n\tuint8_t *buf, \n\t%s *message);\n\n", messages[i].name, messages[i].name);
		fprintf(out, "int protobuf_read_mem_%s(\n\tconst uint8_t *buf, \n\tsize_t len, \n\tsize_t *bytes_read, \n\t%s *message);\n\n", messages[i].name, messages[i].name);
		fprintf(out, "void protobuf_delete_%s(%s *message);\n\n", messages[i].name, messages[i].name);
	}
}

void protoc_write_header_epilogue(FILE *out)
{
	fprintf(out, "#endif\n");
}

void protoc_write_header(
	protoc_message *messages,
	size_t count,
	FILE *out)
{
	protoc_write_header_prologue(messages, count, out);
	protoc_write_header_epilogue(out);
}

void protoc_write_source_utility(FILE *out)
{
	fprintf(out,"\
static inline int protobuf_get_msb64(uint64_t value)\n\
{\n\
	int count = 0;\n\
	while (value >>= 1)\n\
	{\n\
	    count ++;\n\
	}\n\
	return count;\n\
}\n\n\
static inline int protobuf_get_varint_size(uint64_t value)\n\
{\n\
	return protobuf_get_msb64(value)/7 + 1;\n\
}\n\n\
static inline int protobuf_get_tag_size(uint32_t index)\n\
{\n\
	return protobuf_get_msb64(index << 3)/7 + 1;\n\
}\n\n\
static inline int protobuf_decode_varint_size(\n\
	const uint8_t *buffer,\n\
	size_t length)\n\
{\n\
	int result = 0;\n\
	while (length > 0)\n\
	{\n\
		result ++;\n\
		if ((*buffer & 0x80) == 0)\n\
		{\n\
			return result;\n\
		}\n\
		buffer ++;\n\
		length --;\n\
	}\n\
	return -1;\n\
}\n\n\
static inline uint64_t protobuf_decode_varint(const uint8_t *buffer)\n\
{\n\
	uint64_t result = 0;\n\
	int i = 0;\n\
	do\n\
	{\n\
		result |= (*buffer & 0x7F) << i * 7;\n\
		i++;\n\
	} while (*(buffer++) & 0x80);\n\
	return result;\n\
}\n\n\
static inline void protobuf_encode_varint(\n\
	uint8_t **buffer,\n\
	uint64_t value)\n\
{\n\
	do\n\
	{\n\
		**buffer = value & 0x7F | (((value & ~0x7F) != 0) << 7);\n\
		value >>= 7;\n\
		(*buffer) ++;\n\
	} while (value);\n\
}\n\n\
static inline void protobuf_encode_tag(\n\
	uint8_t **buffer,\n\
	int wire,\n\
	uint32_t index)\n\
{\n\
	protobuf_encode_varint(buffer, wire | (index << 3));\n\
}\n\n\
static inline void protobuf_encode_32(\n\
	uint8_t **buffer,\n\
	void *value)\n\
{\n\
	memcpy(*buffer, value, 4);\n\
	(*buffer) += 4;\n\
}\n\n\
");
}

static const protoc_wire_type protoc_type_to_wire[] =
{
	[PROTOC_INT32] = PROTOC_WIRE_VARINT,
	[PROTOC_UINT32] = PROTOC_WIRE_VARINT,
	[PROTOC_BOOL] = PROTOC_WIRE_VARINT,
	[PROTOC_STRING] = PROTOC_WIRE_LEN,
	[PROTOC_FIXED32] = PROTOC_WIRE_I32,
	[PROTOC_FLOAT32] = PROTOC_WIRE_I32
};

void protoc_write_source_buffersize(
	protoc_message *m,
	FILE *out)
{
	size_t i;
	protoc_field *f;
	fprintf(out, "size_t protobuf_buffersize_%s(%s *message)\n{\n", m->name, m->name);
	fprintf(out, "\tsize_t result = 0;\n");
	size_t known_size = 0;
	for (i = 0; i < m->field_count; i++)
	{
		f = m->fields + i;
		if (f->qual == PROTOC_REQUIRED)
		{
			known_size += protoc_get_tag_size(f->index);
			if (f->type == PROTOC_STRING)
			{
				fprintf(out, "\tresult += protobuf_get_varint_size(message->%s_length);\n", f->name);
				fprintf(out, "\tresult += message->%s_length;\n", f->name);
			}
			else if (f->type == PROTOC_FIXED32 || f->type == PROTOC_FLOAT32)
			{
				known_size += 4;
			}
			else
			{
				fprintf(out, "\tresult += protobuf_get_varint_size(message->%s);\n", f->name);
			}
		}
		else if (f->qual == PROTOC_OPTIONAL)
		{
			fprintf(out, "\tresult += %u * (message->%s_exists != 0);\n", protoc_get_tag_size(f->index), f->name);
			if (f->type == PROTOC_STRING)
			{
				fprintf(out, "\tresult += protobuf_get_varint_size(message->%s_length) * (message->%s_exists != 0);\n", f->name, f->name);
				fprintf(out, "\tresult += message->%s_length * (message->%s_exists != 0);\n", f->name, f->name);
			}
			else if (f->type == PROTOC_FIXED32 || f->type == PROTOC_FLOAT32)
			{
				fprintf(out, "\tresult += 4 * (message->%s_exists != 0);\n", f->name);
			}
			else
			{
				fprintf(out, "\tresult += protobuf_get_varint_size(message->%s) * (message->%s_exists != 0);\n", f->name, f->name);
			}
		}
		else if (f->qual == PROTOC_REPEATED)
		{
			fprintf(out, "\tresult += message->%s_count * %u;\n", f->name, protoc_get_tag_size(f->index));
			if (f->type == PROTOC_STRING)
			{
				fprintf(out, "\tfor (size_t i = 0; i < message->%s_count; i++) { result += protobuf_get_varint_size(message->%s_lengths[i]) + message->%s_lengths[i]; }\n", f->name, f->name, f->name);
			}
			else if (f->type == PROTOC_FIXED32 || f->type == PROTOC_FLOAT32)
			{
				fprintf(out, "\tresult += message->%s_count * 4;\n", f->name);
			}
			else
			{
				fprintf(out, "\tresult += message->%s_count * protobuf_get_varint_size(message->%s_lengths[i]);\n", f->name, f->name);
			}
		}
	}
	if (known_size > 0)
	{
		fprintf(out, "\tresult += %zu;\n", known_size);
	}
	fprintf(out, "\treturn result;\n}\n\n");
}

void protoc_write_write_mem_source(
	protoc_message *m,
	FILE *out)
{
	size_t i;
	protoc_field *f;
	fprintf(out, "int protobuf_write_mem_%s(\n\tuint8_t *buf,\n\t%s *message)\n{\n", m->name, m->name);
	for (i = 0; i < m->field_count; i++)
	{
		f = m->fields + i;
		if (f->qual == PROTOC_REQUIRED ||
			f->qual == PROTOC_OPTIONAL)
		{
			if (f->qual == PROTOC_OPTIONAL)
			{
				fprintf(out, "\tif (message->%s_exists)\n\t{\n", f->name);
			}
			fprintf(out, "\tprotobuf_encode_tag(&buf, %d, %d);\n", protoc_type_to_wire[f->type], f->index);
			if (f->type == PROTOC_STRING)
			{
				fprintf(out, "\tprotobuf_encode_varint(&buf, message->%s_length);\n", f->name);
				fprintf(out, "\tmemcpy(buf, message->%s, message->%s_length); buf += message->%s_length;\n", f->name, f->name, f->name);
			}
			else if (f->type == PROTOC_FIXED32 || f->type == PROTOC_FLOAT32)
			{
				fprintf(out, "\tprotobuf_encode_32(&buf, &message->%s);\n", f->name);
			}
			else
			{
				fprintf(out, "\tprotobuf_encode_varint(&buf, message->%s);\n", f->name);
			}
			if (f->qual == PROTOC_OPTIONAL)
			{
				fprintf(out, "\t}\n");
			}
		}
		else if (f->qual == PROTOC_REPEATED)
		{
			fprintf(out, "\tfor (size_t i = 0; i < message->%s_count; i++)\n\t{\n", f->name);
			fprintf(out, "\tprotobuf_encode_tag(&buf, %d, %d);\n", protoc_type_to_wire[f->type], f->index);
			if (f->type == PROTOC_STRING)
			{
				fprintf(out, "\tprotobuf_encode_varint(&buf, message->%s_lengths[i]);\n", f->name);
				fprintf(out, "\tmemcpy(buf, message->%s[i], message->%s_lengths[i]); buf += message->%s_lengths[i];\n", f->name, f->name, f->name);
			}
			else if (f->type == PROTOC_FIXED32 || f->type == PROTOC_FLOAT32)
			{
				fprintf(out, "\tprotobuf_encode_32(&buf, &message->%s[i]);\n", f->name);
			}
			else
			{
				fprintf(out, "\tprotobuf_encode_varint(&buf, message->%s[i]);\n", f->name);
			}
			fprintf(out, "\t}\n");
		}
	}
	fprintf(out, "\treturn 0;\n}\n\n");
}

void protoc_write_read_mem_source(
	protoc_message *m,
	FILE *out)
{
	size_t i;
	protoc_field *f;
	fprintf(out, "int protobuf_read_mem_%s(\n\tconst uint8_t *buf, \n\tsize_t len, \n\tsize_t *bytes_read, \n\t%s *message)\n{\n", m->name, m->name);
	fprintf(out, "\tint length;\n");
	fprintf(out, "\tint wire;\n");
	fprintf(out, "\tuint64_t index;\n");
	fprintf(out, "\tsize_t initial_read;\n");
	fprintf(out, "\tif (*bytes_read == 0)\n\t{\n");
	for (i = 0; i < m->field_count; i++)
	{
		f = m->fields + i;
		if (f->qual == PROTOC_REQUIRED ||
			f->qual == PROTOC_OPTIONAL)
		{
			if (f->qual == PROTOC_OPTIONAL)
			{
				fprintf(out, "\t\tmessage->%s_exists = 0;\n", f->name);
			}
			if (f->type == PROTOC_STRING)
			{
				fprintf(out, "\t\tmessage->%s_length = 0;\n", f->name);
				fprintf(out, "\t\tmessage->%s = NULL;\n", f->name);
			}
			else
			{
				fprintf(out, "\t\tmessage->%s = 0;\n", f->name);
			}
		}
		else
		{
			fprintf(out, "\t\tmessage->%s_count = 0;\n", f->name);
			if (f->type == PROTOC_STRING)
			{
				fprintf(out, "\t\tmessage->%s_lengths = NULL;\n", f->name);
			}
			fprintf(out, "\t\tmessage->%s = NULL;\n", f->name);
		}
	}
	fprintf(out, "\t}\n");
	fprintf(out, "\twhile (*bytes_read < len)\n\t{\n");
	fprintf(out, "\t\tinitial_read = *bytes_read;\n");
	fprintf(out, "\t\tif ((length = protobuf_decode_varint_size(buf, len - *bytes_read)) == -1) { return 1; }\n");
	fprintf(out, "\t\tindex = protobuf_decode_varint(buf); *bytes_read += length; buf += length;\n");
	fprintf(out, "\t\twire = index & 0x7; index >>= 3;\n");
	fprintf(out, "\t\tswitch (index)\n\t\t{\n");
	for (i = 0; i < m->field_count; i++)
	{
		f = m->fields + i;
		fprintf(out, "\t\tcase %d:\n", f->index);
		fprintf(out, "\t\t\tif (wire != %d) { *bytes_read = initial_read; return 3; }\n", protoc_type_to_wire[f->type]);
		if (f->qual == PROTOC_REQUIRED ||
			f->qual == PROTOC_OPTIONAL)
		{
			if (f->type == PROTOC_STRING)
			{
				fprintf(out, "\t\t\tif ((length = protobuf_decode_varint_size(buf, len - *bytes_read)) == -1) { *bytes_read = initial_read; return 1; }\n");
				fprintf(out, "\t\t\tindex = protobuf_decode_varint(buf); *bytes_read += length; buf += length;\n");
				fprintf(out, "\t\t\tif (index + *bytes_read > len) { *bytes_read = initial_read; return 1; }\n");
				fprintf(out, "\t\t\tmessage->%s_length = index;\n", f->name);
				fprintf(out, "\t\t\tmessage->%s = realloc(message->%s, index + 1);\n", f->name, f->name);
				fprintf(out, "\t\t\tmemcpy(message->%s, buf, index);\n", f->name);
				fprintf(out, "\t\t\tmessage->%s[index] = \'\\0\';\n", f->name);
				fprintf(out, "\t\t\t*bytes_read += index; buf += index;\n");
			}
			else if (f->type == PROTOC_FIXED32 || f->type == PROTOC_FLOAT32)
			{
				fprintf(out, "\t\t\tif (*bytes_read + 4 > len) { *bytes_read = initial_read; return 1; }\n");
				fprintf(out, "\t\t\tmemcpy(&message->%s, buf, 4); *bytes_read += 4; buf += 4;\n", f->name);
			}
			else
			{
				fprintf(out, "\t\t\tif ((length = protobuf_decode_varint_size(buf, len - *bytes_read)) == -1) { *bytes_read = initial_read; return 1; }\n");
				fprintf(out, "\t\t\tmessage->%s = protobuf_decode_varint(buf); *bytes_read += length; buf += length;\n", f->name);
			}
			if (f->qual == PROTOC_OPTIONAL)
			{
				fprintf(out, "\t\t\tmessage->%s_exists = 1;\n", f->name);
			}
		}
		else
		{
			if (f->type == PROTOC_STRING)
			{
				fprintf(out, "\t\t\tif ((length = protobuf_decode_varint_size(buf, len - *bytes_read)) == -1) { *bytes_read = initial_read; return 1; }\n");
				fprintf(out, "\t\t\tindex = protobuf_decode_varint(buf); *bytes_read += length; buf += length;\n");
				fprintf(out, "\t\t\tif (index + *bytes_read > len) { *bytes_read = initial_read; return 1; }\n");
				fprintf(out, "\t\t\tmessage->%s = realloc(message->%s, sizeof(*message->%s) * (message->%s_count + 1));\n", f->name, f->name, f->name, f->name);
				fprintf(out, "\t\t\tmessage->%s_lengths = realloc(message->%s_lengths, sizeof(*message->%s_lengths) * (message->%s_count + 1));\n", f->name, f->name, f->name, f->name);
				fprintf(out, "\t\t\tmessage->%s[message->%s_count] = malloc(index + 1);\n", f->name, f->name);
				fprintf(out, "\t\t\tmemcpy(message->%s[message->%s_count], buf, index);\n", f->name, f->name);
				fprintf(out, "\t\t\tmessage->%s[message->%s_count][index] = \'\\0\';\n", f->name, f->name);
				fprintf(out, "\t\t\tmessage->%s_lengths[message->%s_count] = index;\n", f->name, f->name);
				fprintf(out, "\t\t\t*bytes_read += index; buf += index;\n");
			}
			else if (f->type == PROTOC_FIXED32 || f->type == PROTOC_FLOAT32)
			{
				fprintf(out, "\t\t\tif (*bytes_read + 4 > len) { *bytes_read = initial_read; return 1; }\n");
				fprintf(out, "\t\t\tmessage->%s = realloc(message->%s, sizeof(*message->%s) * (message->%s_count + 1));\n", f->name, f->name, f->name, f->name);
				fprintf(out, "\t\t\tmemcpy(message->%s + message->%s_count, buf, 4); *bytes_read += 4; buf += 4;\n", f->name, f->name);
			}
			else
			{
				fprintf(out, "\t\t\tif ((length = protobuf_decode_varint_size(buf, len - *bytes_read)) == -1) { *bytes_read = initial_read; return 1; }\n");
				fprintf(out, "\t\t\tmessage->%s = realloc(message->%s, sizeof(*message->%s) * (message->%s_count + 1));\n", f->name, f->name, f->name, f->name);
				fprintf(out, "\t\t\tmessage->%s[message->%s_count] = protobuf_decode_varint(buf); *bytes_read += length; buf += length;\n", f->name, f->name);
			}
			if (f->qual == PROTOC_OPTIONAL)
			{
				fprintf(out, "\t\t\tmessage->%s_count++;\n", f->name);
			}
		}
		fprintf(out, "\t\t\tbreak;\n");
	}
	fprintf(out, "\t\tdefault:\n");
	fprintf(out, "\t\t\tif (wire == 0) { if ((length = protobuf_decode_varint_size(buf, len - *bytes_read)) == -1) { *bytes_read = initial_read; return 1; } *bytes_read += length; buf += length; }\n");
	fprintf(out, "\t\t\telse if (wire == 1) { if (len - *bytes_read < 8) { *bytes_read = initial_read; return 1; } *bytes_read += 8; buf += 8; }\n");
	fprintf(out, "\t\t\telse if (wire == 5) { if (len - *bytes_read < 4) { *bytes_read = initial_read; return 1; } *bytes_read += 4; buf += 4; }\n");
	fprintf(out, "\t\t\telse if (wire == 2)\n\t\t\t{\n\t\t\t\tif ((length = protobuf_decode_varint_size(buf, len - *bytes_read)) == -1) { *bytes_read = initial_read; return 1; }\n");
	fprintf(out, "\t\t\t\tindex = protobuf_decode_varint(buf); *bytes_read += length; buf += length;\n");
	fprintf(out, "\t\t\t\tif (index + *bytes_read > len) { *bytes_read = initial_read; return 1; }\n");
	fprintf(out, "\t\t\t\t*bytes_read += index;\n\t\t\t}\n");
	fprintf(out, "\t\t\t*bytes_read = initial_read;\n\t\t\treturn 2;\n");
	fprintf(out, "\t\t}\n");
	fprintf(out, "\t}\n\treturn 0;\n}\n\n");
}

void protoc_write_delete_source(
	protoc_message *m,
	FILE *out)
{
	size_t i;
	protoc_field *f;
	fprintf(out, "void protobuf_delete_%s(%s *message)\n{\n\t(void)message;\n", m->name, m->name);
	for (i = 0; i < m->field_count; i++)
	{
		f = m->fields + i;
		if (f->qual == PROTOC_REQUIRED ||
			f->qual == PROTOC_OPTIONAL)
		{
			if (f->type == PROTOC_STRING)
			{
				fprintf(out, "\tfree(message->%s);\n", f->name);
			}
		}
		else if (f->qual == PROTOC_REPEATED)
		{
			if (f->type == PROTOC_STRING)
			{
				fprintf(out, "\tfor (size_t i = 0; i < message->%s_count; i++)\n\t{\n", f->name);
				fprintf(out, "\t\tfree(message->%s[i])\n", f->name);
				fprintf(out, "\t}\n");
			}
			fprintf(out, "\tfree(message->%s);\n", f->name);
		}
	}
	fprintf(out, "}\n\n");
}

void protoc_write_source_sauce(
	protoc_message *messages,
	size_t count,
	FILE *out)
{
	size_t i;
	fputs("#include <stdlib.h>\n", out);
	fputs("\n", out);
	protoc_write_source_utility(out);
	for (i = 0; i < count; i++)
	{
		protoc_write_source_buffersize(messages + i, out);
	}
	for (i = 0; i < count; i++)
	{
		protoc_write_write_mem_source(messages + i, out);
	}
	for (i = 0; i < count; i++)
	{
		protoc_write_read_mem_source(messages + i, out);
	}
	for (i = 0; i < count; i++)
	{
		protoc_write_delete_source(messages + i, out);
	}
}

void protoc_write_source(
	protoc_message *messages,
	size_t count,
	FILE *out,
	const char *header_name)
{
	fprintf(out, "#include \"%s\"\n", header_name);
	protoc_write_source_sauce(messages, count, out);
}

void protoc_write_protocol(protoc_io *io)
{
	if (io->in.merge_messages)
	{
		protoc_write_header_prologue(io->in.messages, io->in.count, io->in.header_out);
		protoc_write_source_sauce(io->in.messages, io->in.count, io->in.header_out);
		protoc_write_header_epilogue(io->in.header_out);
	}
	else
	{
		protoc_write_header(io->in.messages, io->in.count, io->in.header_out);
		protoc_write_source(io->in.messages, io->in.count, io->in.source_out, io->in.header_name);
	}
}