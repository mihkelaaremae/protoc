#include <protoc.h>
#include <protoc_token.h>
#include <protoc_parse.h>
#include <protoc_out.h>
#include <stdlib.h>
#include <string.h>

const char *protoc_error_interpret[] =
{
	"",
	"Cannot open input",
	"Cannot open output",
	"Empty input",
	"Syntax error",
	"Tag value conflict",
};

protoc_error protoc_read_messages(
	FILE *file, 
	protoc_message **messages, 
	size_t *message_count,
	const char *description,
	protoc_io *io)
{
	char *buffer;
	int error;
	protoc_token *tokens;
	size_t length;
	size_t token_count;

	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);
	buffer = malloc(length + 1);
	length = fread(buffer, 1, length, file);

	buffer[length] = '\0';

	if ((error = protoc_tokenize(buffer, &tokens, &token_count, description, io)))
	{
		free(buffer);
		return error;
	}
	if ((error = protoc_parse(messages, message_count, tokens, token_count, io)))
	{
		free(buffer);
		return error;
	}
	free(tokens);
	return PROTOC_SUCCESS;
}

protoc_error protoc_open_files(protoc_io *io)
{
	char *out_name;
	if (io->in.out_prefix == NULL)
	{
		return PROTOC_SUCCESS;
	}
	out_name = malloc(strlen(io->in.out_prefix) + 3);
	strcpy(out_name, io->in.out_prefix);
	out_name[strlen(io->in.out_prefix)] = '.';
	out_name[strlen(io->in.out_prefix) + 1] = 'h';
	out_name[strlen(io->in.out_prefix) + 2] = '\0';
	io->in.header_out = fopen(out_name, "w");
	if (!io->in.header_out)
	{
		return PROTOC_BAD_OUT;
	}
	if (!io->in.merge_messages)
	{
		out_name[strlen(io->in.out_prefix) + 1] = 'c';
		io->in.source_out = fopen(out_name, "w");
		if (!io->in.source_out)
		{
			return PROTOC_BAD_OUT;
		}
	}
	free(out_name);
	return PROTOC_SUCCESS;
}

protoc_error protoc_compile(protoc_io *io)
{
	FILE **files;
	size_t i;
	size_t message_count;
	protoc_message *messages;
	protoc_error error;
	if (io->in.messages == NULL)
	{
		messages = NULL;
		message_count = 0;
		if (io->in.files == NULL)
		{
			if (io->in.count == 0)
			{
				return PROTOC_EMPTY;
			}
			files = malloc(sizeof(*files) * io->in.count);
			for (i = 0; i < io->in.count; i++)
			{
				files[i] = fopen(io->in.filenames[i], "r");
				if (!files[i])
				{
					return PROTOC_BAD_IN;
				}
			}
			for (i = 0; i < io->in.count; i++)
			{
				if ((error = protoc_read_messages(files[i], &messages, &message_count, io->in.filenames[i], io)))
				{
					return error;
				}
			}
			free(files);
		}
		else
		{
			files = io->in.files;
			for (i = 0; i < io->in.count; i++)
			{
				if ((error = protoc_read_messages(files[i], &messages, &message_count, "internal", io)))
				{
					return error;
				}
			}
		}
		io->in.messages = messages;
		io->in.count = message_count;
	}
	if ((error = protoc_open_files(io)))
	{
		return error;
	}
	protoc_write_protocol(io);
	return PROTOC_SUCCESS;
}