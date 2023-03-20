#include <stdio.h>
#include <protocols/example.h>

/*
typedef struct
{
	//1
	char *query;
	size_t query_length;
	 //2
	int32_t page_number;
	int page_number_exists;
	 //3
	int32_t result_per_page;
	int result_per_page_exists;
} SearchRequest;
 */

int main(void)
{
	int error;
	uint8_t *buffer;
	size_t size;
	size_t bytes_read;
	SearchRequest input;
	SearchRequest output;
	const char *input_strings[] =
	{
		"hello world",
		"hello",
		"very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very very long utf8 string öäüõ ```` 🙂😀😃😄😁😅😆🤣😂🙃😉😊😇😎🤓🧐🥳"
	};
	for (int i = 0; i < 10000; i++)
	{
		input.query = (char *)input_strings[i % (sizeof(input_strings) / sizeof(*input_strings))];
		input.query_length = strlen(input.query);
		input.page_number = i * 1054;
		input.page_number_exists = (i % 7) == 0;
		input.result_per_page = i * 45782;
		input.result_per_page_exists = (i % 13) == 0;
		size = protobuf_buffersize_SearchRequest(&input);
		buffer = malloc(size);
		if (error = protobuf_write_mem_SearchRequest(buffer, &input))
		{
			return 1;
		}
		bytes_read = 0;
		if (error = protobuf_read_mem_SearchRequest(buffer, size, &bytes_read, &output))
		{
			return 1;
		}
		free(buffer);
		if (input.page_number_exists != output.page_number_exists ||
			input.result_per_page_exists != output.result_per_page_exists)
		{
			return 1;
		}
		if (input.page_number_exists && input.page_number != output.page_number)
		{
			return 1;
		}
		if (input.result_per_page_exists && input.result_per_page != output.result_per_page)
		{
			return 1;
		}
		if (output.query_length != strlen(input.query) ||
			strcmp(output.query, input.query))
		{
			return 1;
		}
		protobuf_delete_SearchRequest(&output);
	}
	return 0;
}
