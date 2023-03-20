#include <protoc.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char *argv0);

void print_help(const char *argv0);

void print_error(protoc_error error, protoc_io *io);

int main(int argc, char *argv[])
{
	int result;
	int i;
	protoc_io io;
	memset(&io, 0, sizeof(io));
	io.in.out_prefix = "output";
	for (i = 1; i < argc; i++)
	{
		if (strlen(argv[i]) > 0 && argv[i][0] == '-')
		{
			if (strcmp(argv[i], "-h") == 0 ||
				strcmp(argv[i], "-help") == 0 ||
				strcmp(argv[i], "--help") == 0)
			{
				print_help(argv[0]);
				return 0;
			}
			if (strcmp(argv[i], "-o") == 0)
			{
				if (i + 1 >= argc)
				{
					printf("No prefix specified\n");
					return 1;
				}
				io.in.out_prefix = argv[i + 1];
				i++;
			}
			else if (strcmp(argv[i], "-m") == 0)
			{
				io.in.merge_messages = 1;
			}
		}
		else
		{
			io.in.count = argc - i;
			io.in.filenames = (const char **)argv + i;
			result = protoc_compile(&io);
			if (result)
			{
				print_error(result, &io);
				return 1;
			}
			return 0;
		}
	}
	print_usage(argv[0]);
	return 1;
}

void print_usage(const char *argv0)
{
	printf("Usage: %s [-options] *.proto [*.proto *.proto ...]\n", argv0);
	printf("Type '%s -h' for help.", argv0);
}

void print_help(const char *argv0)
{
	printf("Usage: %s [-options] *.proto [*.proto *.proto ...]\n", argv0);
	puts("Options:");
	puts("    -h / --help  Show help");
	puts("    -o prefix    Specify output prefix");
	puts("    -m           Merge output into a single header file");
}

void print_error(protoc_error error, protoc_io *io)
{
	if (error == PROTOC_SYNTAX_ERROR)
	{
		printf("Error at %s:%d:%d %s\n", io->out.relevant_file, io->out.linenum, io->out.colnum, protoc_error_interpret[error]);
	}
	else
	{
		printf("Error: %s\n", protoc_error_interpret[error]);
	}
}
