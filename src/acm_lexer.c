// SPDX-License-Identifier: MIT
// Ape Config Markup
// Copyright © 2020-2025 Mark E Sowden <hogsy@oldtimes-software.com>

//#define ACM_TEST

#include <ctype.h>

#include "acm_private.h"

typedef struct AcmLexerReservedWord
{
	const char  *string;
	AcmTokenType type;
} AcmLexerReservedWord;

static AcmLexerReservedWord reservedWords[] = {
        {"string",  ACM_TOKEN_TYPE_TYPENAME     },
        {"bool",    ACM_TOKEN_TYPE_TYPENAME     },
        {"object",  ACM_TOKEN_TYPE_TYPENAME     },
        {"array",   ACM_TOKEN_TYPE_TYPENAME     },
        {"uint8",   ACM_TOKEN_TYPE_TYPENAME     },
        {"uint16",  ACM_TOKEN_TYPE_TYPENAME     },
        {"uint32",  ACM_TOKEN_TYPE_TYPENAME     },
        {"uint",    ACM_TOKEN_TYPE_TYPENAME     }, // shorthand uint32
        {"uint64",  ACM_TOKEN_TYPE_TYPENAME     },
        {"int8",    ACM_TOKEN_TYPE_TYPENAME     },
        {"int16",   ACM_TOKEN_TYPE_TYPENAME     },
        {"int32",   ACM_TOKEN_TYPE_TYPENAME     },
        {"int",     ACM_TOKEN_TYPE_TYPENAME     }, // shorthand int32
        {"int64",   ACM_TOKEN_TYPE_TYPENAME     },
        {"float",   ACM_TOKEN_TYPE_TYPENAME     },
        {"float64", ACM_TOKEN_TYPE_TYPENAME     },

        {"{",       ACM_TOKEN_TYPE_OPEN_BRACKET },
        {"}",       ACM_TOKEN_TYPE_CLOSE_BRACKET},
};
static const unsigned int NUM_RESERVED_WORDS = ( sizeof( reservedWords ) / sizeof( *( reservedWords ) ) );

#define NOT_TERMINATING_CHAR( P ) ( ( P ) != '\0' && ( P ) != '\n' && ( P ) != '\r' )

static int get_line_end( const char *p )
{
	if ( *p == '\0' ) return 0;
	if ( *p == '\n' ) return 1;
	if ( *p == '\r' && *( p + 1 ) == '\n' ) return 2;

	return -1;
}

static bool is_line_end( const char *p )
{
	return get_line_end( p ) != -1;
}

static bool is_whitespace( const char *p )
{
	if ( is_line_end( p ) )
	{
		return false;
	}

	return isspace( *p ) || *p == '\t';
}

static void skip_whitespace( const char **p )
{
	if ( !is_whitespace( *p ) )
	{
		return;
	}

	do
	{
		( *p )++;
	} while ( is_whitespace( *p ) );
}

static void skip_line( const char **p )
{
	while ( NOT_TERMINATING_CHAR( *( *p ) ) ) ( *p )++;
	if ( *( *p ) == '\r' ) ( *p )++;
	if ( *( *p ) == '\n' ) ( *p )++;
}

static unsigned int get_token_length( const char *p )
{
	skip_whitespace( &p );

	const char *s = p;
	while ( *p != '\0' && *p != ' ' )
	{
		if ( is_line_end( p ) )
		{
			break;
		}

		p++;
	}

	return p - s;
}

static const char *read_token( const char **p, char *dst, size_t size )
{
	skip_whitespace( p );

	unsigned int length = get_token_length( ( *p ) );
	size_t       i;
	for ( i = 0; i < length; ++i )
	{
		if ( ( i + 1 ) >= size )
		{
			break;
		}

		dst[ i ] = ( *p )[ i ];
	}

	( *p ) += length;

	dst[ i ] = '\0';
	return dst;
}

static unsigned int get_enclosed_string_length( const char *p )
{
	skip_whitespace( &p );

	const char *s          = p;
	bool        isEnclosed = false;
	if ( *p == '\"' )
	{
		p++;
		isEnclosed = true;
	}

	while ( NOT_TERMINATING_CHAR( *p ) )
	{
		if ( !isEnclosed && ( *p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' ) )
		{
			break;
		}
		if ( isEnclosed && *p == '\"' )
		{
			p++;
			break;
		}

		p++;
	}

	return p - s;
}

static const char *read_enclosed_string( const char **p, char *dst, size_t size )
{
	skip_whitespace( p );

	bool isEnclosed = false;
	if ( *( *p ) == '\"' )
	{
		( *p )++;
		isEnclosed = true;
	}

	size_t i = 0;
	while ( NOT_TERMINATING_CHAR( *( *p ) ) )
	{
		if ( !isEnclosed && ( *( *p ) == ' ' || *( *p ) == '\t' || *( *p ) == '\n' || *( *p ) == '\r' ) )
		{
			break;
		}

		if ( isEnclosed && *( *p ) == '\"' )
		{
			( *p )++;
			break;
		}

		if ( ( i + 1 ) < size )
		{
			dst[ i++ ] = *( *p );
		}
		( *p )++;
	}

	dst[ i ] = '\0';
	return dst;
}

static unsigned int get_line_length( const char *p )
{
	unsigned int length = 0;
	while ( NOT_TERMINATING_CHAR( *p ) )
	{
		length++;
		p++;
	}

	return length;
}

static const char *read_line( const char **p, char *dst, size_t size )
{
	unsigned int length = get_line_length( *p );
	size_t       i;
	for ( i = 0; i < length; ++i )
	{
		if ( ( i + 1 ) >= size )
		{
			break;
		}

		dst[ i ] = ( *p )[ i ];
	}

	( *p ) += length + get_line_end( ( *p ) + length );

	dst[ i ] = '\0';
	return dst;
}

static AcmTokenType get_token_type_for_symbol( const char *symbol )
{
	if ( *symbol == '\0' )
	{
		return ACM_TOKEN_TYPE_EOF;
	}

	for ( unsigned int i = 0; i < NUM_RESERVED_WORDS; ++i )
	{
		const char *p = reservedWords[ i ].string;
		if ( strcmp( p, symbol ) != 0 )
		{
			continue;
		}

		return reservedWords[ i ].type;
	}

	return ACM_TOKEN_TYPE_IDENTIFIER;
}

static void parse_line( const char *p, const char *file, unsigned int lineNum, AcmLexer *lexer )
{
	const char *o = p;
	while ( true )
	{
		skip_whitespace( &p );

		if ( *p == '\0' )
		{
			break;
		}

		if ( *p == ';' )
		{
			if ( *( p + 1 ) == '*' )
			{
				// multi-line comment
				p += 2;
				while ( *p != '*' && *( p + 1 ) != ';' )
				{
					int l = get_line_end( p );
					if ( l != -1 )
					{
						p += l;
						continue;
					}

					p++;
				}
				p += 2;
				continue;
			}

			// single-line comment
			break;
		}

		AcmTokenType type = ACM_TOKEN_TYPE_INVALID;

		unsigned int linePos = ( p - o ) + 1;
		unsigned int symbolSize;
		char        *symbol;
		if ( *p == '\"' )
		{
			symbolSize = get_enclosed_string_length( p ) + 1;
			symbol     = ACM_NEW_( char, symbolSize );
			if ( read_enclosed_string( &p, symbol, symbolSize ) == NULL )
			{
				Warning( "Failed to parse enclosed string: %u:%u\n", lineNum, linePos );
				break;
			}

			type = ACM_TOKEN_TYPE_STRING;
		}
		else
		{
			symbolSize = get_token_length( p ) + 1;
			symbol     = ACM_NEW_( char, symbolSize );
			if ( read_token( &p, symbol, symbolSize ) == NULL )
			{
				Warning( "Failed to parse token for lexer: %u:%u\n", lineNum, linePos );
				break;
			}

			if ( isdigit( *symbol ) || *symbol == '-' )
			{
				type = ACM_TOKEN_TYPE_INTEGER;
				for ( unsigned int i = 0; i < symbolSize; ++i )
				{
					if ( symbol[ i ] == '.' )
					{
						if ( type == ACM_TOKEN_TYPE_DECIMAL )
						{
							Warning( "Unexpected token in num: %u:%u\n", lineNum, linePos );
							break;
						}

						type = ACM_TOKEN_TYPE_DECIMAL;
					}
				}
			}
			else
			{
				type = get_token_type_for_symbol( symbol );
			}
		}

		if ( type != ACM_TOKEN_TYPE_INVALID )
		{
			AcmLexerToken *token = ACM_NEW( AcmLexerToken );
			snprintf( token->symbol, sizeof( token->symbol ), "%s", symbol );
			snprintf( token->path, sizeof( token->path ), "%s", file );//TODO: can be simplified!!!
			token->lineNum = lineNum;
			token->linePos = linePos;
			token->type    = type;

			if ( lexer->end != NULL )
			{
				lexer->end->next = token;
				token->prev      = lexer->end;
			}

			lexer->end = token;
			if ( lexer->start == NULL )
			{
				lexer->start = lexer->end;
			}

			ACM_DELETE( symbol );
		}
		else
		{
			Warning( "Unexpected character: %u:%u\n", lineNum, linePos );
		}
	}
}

AcmLexer *acm_lexer_parse_buffer_( AcmLexer *self, const char *buf, const char *file )
{
	if ( self == NULL )
	{
		self = ACM_NEW( AcmLexer );
		if ( self == NULL )
		{
			return NULL;
		}

		snprintf( self->originPath, sizeof( self->originPath ), "%s", file );
	}

#if defined( ACM_TEST )
	double startTime = PlGetCurrentSeconds();
#endif

	unsigned int curLineNum = 0;
	const char  *p          = buf;
	while ( *p != '\0' )
	{
		curLineNum++;

		if ( *p == ';' )
		{
			if ( *( p + 1 ) == '*' )
			{
				// multi-line comment
				p += 2;
				while ( *p != '*' && *( p + 1 ) != ';' )
				{
					int l = get_line_end( p );
					if ( l != -1 )
					{
						p += l;
						continue;
					}

					p++;
				}
				p += 2;
				continue;
			}

			// single-line comment
			skip_line( &p );
			continue;
		}

		// tokenise the line
		unsigned int bufSize = get_line_length( p ) + 1;
		if ( bufSize > 1 )
		{
			char *line = ACM_NEW_( char, bufSize );
			read_line( &p, line, bufSize );

			parse_line( line, file, curLineNum, self );

			ACM_DELETE( line );
		}
		else
		{
			skip_line( &p );
		}
	}

#if defined( ACM_TEST )
	// output the result from the lexer
	printf( "%5s %20s %10s %10s\n", "TYPE", "SYMBOL", "LINE", "LPOS" );
	const AcmLexerToken *token;
	PL_ITERATE_LINKED_LIST( token, AcmLexerToken, self->tokens, i )
	{
		printf( "%5d %20s %10u %10u\n", token->type, token->symbol, token->lineNum, token->linePos );
	}

	double endTime = PlGetCurrentSeconds();
	printf( "Lexer took " PL_FMT_double "s for \"%s\"\n", endTime - startTime, file );
#endif

	return self;
}
