#include <protoc_token.h>
#include <stdlib.h>
#include <string.h>

void protoc_adjust_type(protoc_token *token)
{
	static const char *keywords[] = 
	{
		"message",
		"required",
		"optional",
		"repeated",
		"int32",
		"uint32",
		"bool",
		"string",
		"fixed32",
		"float",
	};
	size_t i;
	for (i = 0; i < sizeof(keywords)/sizeof(*keywords); i++)
	{
		if (strcmp(token->token, keywords[i]) == 0)
		{
			token->type = i;
			break;
		}
	}
}

protoc_token_type protoc_get_type(int character)
{
	if (character >= '0' && 
		character <= '9')
	{
		return PROTOC_TOK_NUMBER;
	}
	if (
		(character >= 'a' && 
			character <= 'z') ||
		(character >= 'A' && 
			character <= 'Z') ||
		(character >= '_' && 
			character <= '_'))
	{
		return PROTOC_TOK_LITERAL;
	}
	switch (character)
	{
		case '{': return PROTOC_TOK_CURLY_OPEN;
		case '}': return PROTOC_TOK_CURLY_CLOSE;
		case '=': return PROTOC_TOK_EQUALS;
		case ';': return PROTOC_TOK_SEMICOLON;
		case '/': return PROTOC_TOK_LINECOMMENT;
	}
	return PROTOC_TOK_WHITESPACE;
}

protoc_error protoc_tokenize(
	const char *string, 
	protoc_token **tokens, 
	size_t *count,
	const char *description,
	protoc_io *io)
{
	(void)io;

#define APPEND_TOKEN	(*tokens)[*count].token = malloc(string - start + 1);\
						memcpy((*tokens)[*count].token, start, (uintptr_t)(string - start));\
						(*tokens)[*count].token[(uintptr_t)(string - start)] = 0;\
						(*count) ++;\
						if (*count >= capacity)\
						{\
							capacity *= 2;\
							*tokens = realloc(*tokens, sizeof(**tokens) * capacity);\
						}\
						(*tokens)[*count].type = PROTOC_TOK_INVALID;\
						(*tokens)[*count].line = line;\
						(*tokens)[*count].col = col;\
						(*tokens)[*count].description = description;\
						start = string;\
						state = 0;

#define DISCARD_TOKEN	(*tokens)[*count].type = PROTOC_TOK_INVALID;\
						(*tokens)[*count].line = line;\
						(*tokens)[*count].col = col-1;\
						(*tokens)[*count].description = description;\
						start = string;\
						state = 0;

	const char *start = string;
	int state = 0;
	size_t capacity = 16;
	*count = 0;
	*tokens = malloc(sizeof(**tokens) * capacity);
	(*tokens)[*count].type = PROTOC_TOK_INVALID;
	(*tokens)[*count].line = 1;
	(*tokens)[*count].col = 1;
	(*tokens)[*count].description = description;
	int col = 1;
	int line = 1;
	while (*string)
	{
		switch ((*tokens)[*count].type)
		{
			case PROTOC_TOK_INVALID:
				(*tokens)[*count].type = protoc_get_type(*string);
				break;
			case PROTOC_TOK_NUMBER:
				if (!(*string >= '0' && 
					*string <= '9'))
				{
					APPEND_TOKEN
					continue;
				}
				break;
			case PROTOC_TOK_LITERAL:
				if (!(
					(*string >= 'a' && 
						*string <= 'z') ||
					(*string >= 'A' && 
						*string <= 'Z') ||
					(*string >= '_' && 
						*string <= '_') ||
					(*string >= '0' && 
						*string <= '9')))
				{
					APPEND_TOKEN
					protoc_adjust_type((*tokens) + (*count) - 1);
					continue;
				}
				break;
			case PROTOC_TOK_CURLY_OPEN:
			case PROTOC_TOK_CURLY_CLOSE:
			case PROTOC_TOK_EQUALS:
			case PROTOC_TOK_SEMICOLON:
				APPEND_TOKEN
				continue;
				break;
			case PROTOC_TOK_WHITESPACE:
				if (protoc_get_type(*string) != PROTOC_TOK_WHITESPACE)
				{
					DISCARD_TOKEN
					continue;
				}
				break;
			case PROTOC_TOK_LINECOMMENT:
				if (state == 0)
				{
					if (*string == '/')
					{
						state = 1;
					}
					else if (*string == '*')
					{
						(*tokens)[*count].type = PROTOC_TOK_BLOCKCOMMENT;
					}
					else
					{
						APPEND_TOKEN
					}
				}
				else
				{
					if (*string == '\n')
					{
						string ++;
						if (*string == '\n')
						{
							col = 1;
							line ++;
						}
						else
						{
							col ++;
						}
						DISCARD_TOKEN
						continue;
					}
				}
				break;
			case PROTOC_TOK_BLOCKCOMMENT:
				if (state == 0)
				{
					if (*string == '*')
					{
						state = 1;
					}
				}
				else
				{
					if (*string == '/')
					{
						string ++;
						if (*string == '\n')
						{
							col = 1;
							line ++;
						}
						else
						{
							col ++;
						}
						DISCARD_TOKEN
						continue;
					}
					else
					{
						state = 0;
					}
				}
				break;

			default:
				break;
		}
		string ++;
		if (*string == '\n')
		{
			col = 1;
			line ++;
		}
		else
		{
			col ++;
		}
	}
	switch ((*tokens)[*count].type)
	{
		case PROTOC_TOK_INVALID:
		case PROTOC_TOK_WHITESPACE:
		case PROTOC_TOK_LINECOMMENT:
		case PROTOC_TOK_BLOCKCOMMENT:
			DISCARD_TOKEN
			break;
		case PROTOC_TOK_LITERAL:
			APPEND_TOKEN
			protoc_adjust_type((*tokens) + (*count) - 1);
			break;
		default:
			APPEND_TOKEN
			break;
	}
	return PROTOC_SUCCESS;

#undef APPEND_TOKEN
#undef DISCARD_TOKEN

}