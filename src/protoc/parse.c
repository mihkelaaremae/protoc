#include <protoc_parse.h>
#include <stdlib.h>
#include <string.h>

typedef enum
{
	PROTOC_PARSESTATE_MESSAGE,		//Editing topmost message
	PROTOC_PARSESTATE_FIELD			//Editing topmost field
} protoc_parsestate;

typedef enum
{
	PROTOC_PARSEVARIANT_SINGLE,		//State contains single variant
	PROTOC_PARSEVARIANT_END,		//}
	PROTOC_PARSEVARIANT_FIELD		//Field declaration
} protoc_parsevariant;

typedef struct
{
	protoc_parsestate state;
	protoc_parsestate next_state;
	protoc_parsevariant variant;
	protoc_token_type *tokens;
} protoc_parsehelper;

static const protoc_parsehelper protoc_grammar[] = 
{
	{
		PROTOC_PARSESTATE_MESSAGE, 
		PROTOC_PARSESTATE_FIELD,
		PROTOC_PARSEVARIANT_SINGLE,
		(protoc_token_type[])
		{
			PROTOC_TOK_KEYWORD_MESSAGE,
			PROTOC_TOK_LITERAL,
			PROTOC_TOK_CURLY_OPEN,
			PROTOC_SPECIAL_END
		}
	},
	{
		PROTOC_PARSESTATE_FIELD, 
		PROTOC_PARSESTATE_MESSAGE,
		PROTOC_PARSEVARIANT_END,
		(protoc_token_type[])
		{
			PROTOC_TOK_CURLY_CLOSE,
			PROTOC_SPECIAL_END
		}
	},
	{
		PROTOC_PARSESTATE_FIELD, 
		PROTOC_PARSESTATE_FIELD,
		PROTOC_PARSEVARIANT_FIELD,
		(protoc_token_type[])
		{
			PROTOC_SPECIAL_RANGE, PROTOC_TOK_KEYWORD_REQUIRED, PROTOC_TOK_KEYWORD_REPEATED,
			PROTOC_SPECIAL_RANGE, PROTOC_TOK_KEYWORD_INT32, PROTOC_TOK_KEYWORD_FLOAT,
			PROTOC_TOK_LITERAL,
			PROTOC_TOK_EQUALS,
			PROTOC_TOK_NUMBER,
			PROTOC_TOK_SEMICOLON,
			PROTOC_SPECIAL_END
		}
	}
};

protoc_error protoc_parse(
	protoc_message **messages,
	size_t *message_count,
	protoc_token *tokens, 
	size_t token_count,
	protoc_io *io)
{
	static const protoc_message_type token_to_type[] = 
	{
		[PROTOC_TOK_KEYWORD_INT32] = PROTOC_INT32,
		[PROTOC_TOK_KEYWORD_UINT32] = PROTOC_UINT32,
		[PROTOC_TOK_KEYWORD_BOOL] = PROTOC_BOOL,
		[PROTOC_TOK_KEYWORD_STRING] = PROTOC_STRING,
		[PROTOC_TOK_KEYWORD_FIXED32] = PROTOC_FIXED32,
		[PROTOC_TOK_KEYWORD_FLOAT] = PROTOC_FLOAT32
	};

	static const protoc_message_qualifier token_to_qual[] = 
	{
		[PROTOC_TOK_KEYWORD_REQUIRED] = PROTOC_REQUIRED,
		[PROTOC_TOK_KEYWORD_OPTIONAL] = PROTOC_OPTIONAL,
		[PROTOC_TOK_KEYWORD_REPEATED] = PROTOC_REPEATED
	};

	size_t grammar = 0;
	size_t j = 0;
	size_t k = 0;
	protoc_parsestate state = PROTOC_PARSESTATE_MESSAGE;
	protoc_message *m;
	while (token_count)
	{
		io->out.relevant_file = NULL;
		for (grammar = 0; grammar < sizeof(protoc_grammar)/sizeof(*protoc_grammar); grammar++)
		{
			if (protoc_grammar[grammar].state != state)
			{
				continue;
			}
			k = 0;
			for (j = 0; protoc_grammar[grammar].tokens[j] != PROTOC_SPECIAL_END; j++, k++)
			{
				if (k >= token_count)
				{
					break;
				}
				if (protoc_grammar[grammar].tokens[j] == PROTOC_SPECIAL_RANGE)
				{
					if (tokens[k].type >= protoc_grammar[grammar].tokens[j + 1] &&
						tokens[k].type <= protoc_grammar[grammar].tokens[j + 2])
					{
						j += 2;
					}
					else
					{
						io->out.relevant_file = tokens[k].description;
						io->out.linenum = tokens[k].line;
						io->out.colnum = tokens[k].col;
						break;
					}
				}
				else
				{
					if (tokens[k].type != protoc_grammar[grammar].tokens[j])
					{
						io->out.relevant_file = tokens[k].description;
						io->out.linenum = tokens[k].line;
						io->out.colnum = tokens[k].col;
						break;
					}
				}
			}
			if (protoc_grammar[grammar].tokens[j] == PROTOC_SPECIAL_END)
			{
				break;
			}
		}
		if (grammar == sizeof(protoc_grammar)/sizeof(*protoc_grammar))
		{
			if (*messages)
			{
				free(*messages);
				*messages = NULL;
				*message_count = 0;
			}
			if (!io->out.relevant_file)
			{
				io->out.relevant_file = tokens->description;
				io->out.linenum = tokens->line;
				io->out.colnum = tokens->col;
			}
			return PROTOC_SYNTAX_ERROR;
		}
		switch (protoc_grammar[grammar].state)
		{
			case PROTOC_PARSESTATE_MESSAGE:
				(*message_count)++;
				if (*messages == NULL)
				{
					*messages = malloc(sizeof(**messages));
				}
				else
				{
					*messages = realloc(*messages, sizeof(**messages) * (*message_count));
				}
				m = (*messages) + (*message_count) - 1;
				m->name = malloc(strlen(tokens[1].token) + 1);
				strcpy(m->name, tokens[1].token);
				m->fields = NULL;
				m->field_count = 0;
				break;
			case PROTOC_PARSESTATE_FIELD:
				if (protoc_grammar[grammar].variant == PROTOC_PARSEVARIANT_FIELD)
				{
					m = (*messages) + (*message_count) - 1;
					m->field_count ++;
					if (m->fields == NULL)
					{
						m->fields = malloc(sizeof(*(m->fields)));
					}
					else
					{
						m->fields = realloc(m->fields, sizeof(*m->fields) * (m->field_count));
					}
					m->fields[m->field_count - 1].qual = token_to_qual[tokens[0].type];
					m->fields[m->field_count - 1].type = token_to_type[tokens[1].type];
					m->fields[m->field_count - 1].name = malloc(strlen(tokens[2].token) + 1);
					strcpy(m->fields[m->field_count - 1].name, tokens[2].token);
					m->fields[m->field_count - 1].index = atoi(tokens[4].token);
					for (j = 0; j < m->field_count - 1; j++)
					{
						if (m->fields[j].index == m->fields[m->field_count - 1].index)
						{
							io->out.relevant_file = tokens->description;
							io->out.linenum = tokens->line;
							io->out.colnum = tokens->col;
							return PROTOC_TAG_CONFLICT;
						}
					}
				}
				break;
		}
		state = protoc_grammar[grammar].next_state;
		tokens += k;
		token_count -= k;
	}
	return PROTOC_SUCCESS;
}