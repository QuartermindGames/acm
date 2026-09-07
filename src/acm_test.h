// SPDX-License-Identifier: MIT
// Another Config Markup
// Copyright © 2020-2026 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// If this looks familiar, it's because it's pulled from my test header for qmfw :)

enum
{
	TEST_RETURN_SUCCESS,
	TEST_RETURN_FAILURE,
	TEST_RETURN_FATAL,
};

#define TEST_RUN_START \
	bool allPassed = true;
#define TEST_RUN_END                                        \
	if ( allPassed )                                        \
	{                                                       \
		printf( "\nAll tests passed successfully!\n" );     \
		return EXIT_SUCCESS;                                \
	}                                                       \
	else                                                    \
	{                                                       \
		printf( "\nReached end but some tests failed!\n" ); \
		return EXIT_FAILURE;                                \
	}

#define TEST_FUNC( NAME )       \
	uint8_t test_##NAME( void ) \
	{                           \
		printf( " " #NAME "...\t" );
#define TEST_FUNC_END()         \
	printf( "OK\n" );       \
	return TEST_RETURN_SUCCESS; \
	}
#define CALL_FUNC_TEST( NAME )                                       \
	{                                                                \
		int ret = test_##NAME();                                     \
		if ( ret != TEST_RETURN_SUCCESS )                            \
		{                                                            \
			fprintf( stderr, "Failed on " #NAME "!\n" );             \
			if ( ret == TEST_RETURN_FATAL ) { return EXIT_FAILURE; } \
			allPassed = false;                                       \
		}                                                            \
	}

#define TEST_FAIL( ... )   \
	printf( __VA_ARGS__ ); \
	return TEST_RETURN_FAILURE
#define TEST_ASSERT( TEST )                            \
	if ( !( TEST ) )                                   \
	{                                                  \
		printf( "Assert failed on \"" #TEST "\"!\n" ); \
		return TEST_RETURN_FAILURE;                    \
	}
