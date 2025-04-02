// SPDX-License-Identifier: MIT
// Ape Config Markup
// Copyright © 2020-2025 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined( __cplusplus )
extern "C"
{
#endif

	// override these if you want to provide your own alloc methods
	// just mind that ACM expects memory to be zero'd!!
#ifndef ACM_NEW_
#	define ACM_NEW_( TYPE, NUM ) ( TYPE * ) calloc( NUM, sizeof( TYPE ) )
#endif
#ifndef ACM_NEW
#	define ACM_NEW( TYPE ) ACM_NEW_( TYPE, 1 )
#endif
#ifndef ACM_REALLOC
#	define ACM_REALLOC( PTR, TYPE, NUM ) ( TYPE * ) realloc( PTR, NUM * sizeof( TYPE ) )
#endif
#ifndef ACM_DELETE
#	define ACM_DELETE( PTR ) free( PTR )
#endif

	typedef struct AcmBranch AcmBranch;

	// these are just simple helpers for code structuring
	typedef AcmBranch *( *AcmSerializeFunction )( void *ptr, AcmBranch *root );
	typedef void *( *AcmDeserializeFunction )( void *ptr, AcmBranch *root );

#define ACM_DEFAULT_EXTENSION_OLD ".n"//TODO: should eventually remove this once ApeTech scripts are updated...
#define ACM_DEFAULT_EXTENSION     ".acm"

	typedef enum AcmErrorCode
	{
		ND_ERROR_SUCCESS,

		ND_ERROR_IO_READ,  /* read failure */
		ND_ERROR_IO_WRITE, /* write failure */

		NL_ERROR_MEM_ALLOC, /* alloc failure */

		ND_ERROR_INVALID_ARGUMENT,
		ND_ERROR_INVALID_TYPE,     /* invalid node parent/child type */
		ND_ERROR_INVALID_ELEMENTS, /* unexpected number of elements */
	} AcmErrorCode;

	typedef enum AcmFileType
	{
		ACM_FILE_TYPE_INVALID = -1,
		ACM_FILE_TYPE_BINARY,
		ACM_FILE_TYPE_UTF8,
	} AcmFileType;

	typedef enum AcmPropertyType
	{
		ACM_PROPERTY_TYPE_INVALID = -1,

		// special types
		ACM_PROPERTY_TYPE_OBJECT,
		ACM_PROPERTY_TYPE_ARRAY,
		ACM_PROPERTY_TYPE_STRING,

		// standard types
		ACM_PROPERTY_TYPE_BOOL,
		ACM_PROPERTY_TYPE_FLOAT32,// float
		ACM_PROPERTY_TYPE_FLOAT64,// double
		ND_PROPERTY_INT8,         // int8
		ND_PROPERTY_INT16,        // int16
		ND_PROPERTY_INT32,        // int32
		ND_PROPERTY_INT64,        // int64
		ND_PROPERTY_UI8,          // uint8
		ND_PROPERTY_UI16,         // uint16
		ND_PROPERTY_UI32,         // uint32
		ND_PROPERTY_UI64,         // uint64

		ACM_MAX_PROPERTY_TYPES
	} AcmPropertyType;

	const char  *acm_get_error_message( void );
	AcmErrorCode acm_get_error( void );

	unsigned int acm_get_num_of_children( const AcmBranch *self ); /* only valid for object/array */
	AcmBranch   *acm_get_first_child( AcmBranch *self );
	AcmBranch   *acm_get_child_by_name( AcmBranch *self, const char *name ); /* only valid for object */
	AcmBranch   *acm_get_parent( AcmBranch *self );
	AcmBranch   *acm_get_next_child( AcmBranch *node );

	const char     *acm_branch_get_name( const AcmBranch *self );
	AcmPropertyType acm_branch_get_type( const AcmBranch *self );

	AcmErrorCode acm_branch_get_bool( const AcmBranch *self, bool *dest );
	AcmErrorCode acm_branch_get_string( const AcmBranch *self, char *dest, size_t length );
	AcmErrorCode acm_branch_get_float32( const AcmBranch *self, float *dest );
	AcmErrorCode acm_branch_get_float64( const AcmBranch *self, double *dest );
	AcmErrorCode acm_branch_get_int8( const AcmBranch *self, int8_t *dest );
	AcmErrorCode acm_branch_get_int16( const AcmBranch *self, int16_t *dest );
	AcmErrorCode acm_branch_get_int32( const AcmBranch *self, int32_t *dest );
	AcmErrorCode acm_branch_get_int64( const AcmBranch *self, int64_t *dest );
	AcmErrorCode acm_branch_get_uint8( const AcmBranch *self, uint8_t *dest );
	AcmErrorCode acm_branch_get_uint16( const AcmBranch *self, uint16_t *dest );
	AcmErrorCode acm_branch_get_uint32( const AcmBranch *self, uint32_t *dest );
	AcmErrorCode acm_branch_get_uint64( const AcmBranch *self, uint64_t *dest );

	bool        acm_get_bool( AcmBranch *root, const char *name, bool fallback );
	const char *acm_get_string( AcmBranch *node, const char *name, const char *fallback );

	float  acm_get_f32( AcmBranch *node, const char *name, float fallback );
	double acm_get_f64( AcmBranch *node, const char *name, double fallback );

	intmax_t  acm_branch_get_child_int( AcmBranch *root, const char *name, intmax_t fallback );
	uintmax_t acm_get_uint( AcmBranch *root, const char *name, uintmax_t fallback );

#define ACM_GET_INT( VAR, ROOT, NAME, FALLBACK )  ( typeof( ( VAR ) ) ) acm_branch_get_child_int( ( ROOT ), ( NAME ), ( FALLBACK ) )
#define ACM_GET_UINT( VAR, ROOT, NAME, FALLBACK ) ( typeof( ( VAR ) ) ) acm_get_uint( ( ROOT ), ( NAME ), ( FALLBACK ) )

	AcmErrorCode acm_branch_get_bool_array( AcmBranch *self, bool *buf, unsigned int numElements );
	AcmErrorCode acm_branch_get_string_array( AcmBranch *self, char **buf, unsigned int numElements );
	AcmErrorCode acm_branch_get_int16_array( AcmBranch *self, int16_t *buf, unsigned int numElements );
	AcmErrorCode acm_branch_get_int32_array( AcmBranch *self, int32_t *buf, unsigned int numElements );
	AcmErrorCode acm_branch_get_uint32_array( AcmBranch *self, uint32_t *buf, unsigned int numElements );
	AcmErrorCode acm_branch_get_float32_array( AcmBranch *self, float *buf, unsigned int numElements );
	AcmErrorCode acm_branch_get_float64_array( AcmBranch *self, double *buf, unsigned int numElements );

	int16_t *acm_get_array_i16( AcmBranch *branch, const char *name, int16_t *destination, unsigned int numElements );
	float   *acm_get_array_f32( AcmBranch *branch, const char *name, float *destination, uint32_t numElements );

	AcmBranch *acm_linear_lookup( AcmBranch *root, const char *name );

	AcmBranch *acm_push_branch( AcmBranch *parent, AcmBranch *child );
	AcmBranch *acm_push_object( AcmBranch *node, const char *name );
	AcmBranch *acm_push_string( AcmBranch *parent, const char *name, const char *var, bool conditional );
	AcmBranch *acm_push_bool( AcmBranch *parent, const char *name, bool var );
	AcmBranch *acm_push_i8( AcmBranch *parent, const char *name, int8_t var );
	AcmBranch *acm_push_i16( AcmBranch *parent, const char *name, int16_t var );
	AcmBranch *acm_push_i32( AcmBranch *parent, const char *name, int32_t var );
	AcmBranch *acm_push_ui32( AcmBranch *parent, const char *name, uint32_t var );
	AcmBranch *acm_push_f32( AcmBranch *parent, const char *name, float var );
	AcmBranch *acm_push_f64( AcmBranch *parent, const char *name, double var );

	AcmBranch *acm_push_array_object( AcmBranch *parent, const char *name );
	AcmBranch *acm_push_array_string( AcmBranch *parent, const char *name, const char **array, unsigned int numElements );
	AcmBranch *acm_push_array_i16( AcmBranch *root, const char *name, const int16_t *array, unsigned int numElements );
	AcmBranch *acm_push_array_i32( AcmBranch *parent, const char *name, const int32_t *array, unsigned int numElements );
	AcmBranch *acm_push_array_ui32( AcmBranch *parent, const char *name, const uint32_t *array, unsigned int numElements );
	AcmBranch *acm_push_array_f32( AcmBranch *parent, const char *name, const float *array, unsigned int numElements );

	bool acm_set_variable( AcmBranch *root, const char *name, const char *value, AcmPropertyType type, bool createOnFail );

	/**
	 * Returns a copy of the given branch, inc. all its children.
	 *
	 * @param node 	Pointer to the branch you want copied.
	 * @return 		Pointer to the new copy, Null on failure.
	 */
	AcmBranch *acm_copy_branch( AcmBranch *node );

	/**
	 * Destroy the given branch and all its children.
	 *
	 * @param node	Pointer to the branch you want to destroy.
	 */
	void acm_branch_destroy( AcmBranch *node );

	/**
	 * Load a given file from a buffer.
	 *
	 * @param buf 			Pointer to null-terminated buffer.
	 * @param bufSize
	 * @param objectType 	Expected root object type, can be left null.
	 * @param source		Source file we loaded from.
	 * @return 				Pointer to the root branch. Null on failure.
	 */
	AcmBranch *acm_load_from_memory( const void *buf, size_t bufSize, const char *objectType, const char *source );

	/**
	 * Load a file via the given path.
	 *
	 * @param path 			Path to load from.
	 * @param objectType 	Expected root object type, can be left null.
	 * @return 				Pointer to the root branch. Null on failure.
	 */
	AcmBranch *acm_load_file( const char *path, const char *objectType );

	/**
	 * Writes the given branch to the destination.
	 *
	 * @param path		Output location.
	 * @param root 		Branch to serialise.
	 * @param fileType 	Type of file to write out (either binary / utf8).
	 * @return 			True on success, false on failure.
	 */
	bool acm_write_file( const char *path, AcmBranch *root, AcmFileType fileType );

	/**
	 * Parse a null-terminated buffer.
	 *
	 * @param buf 	Buffer to parse.
	 * @param file 	Origin file, can be left null.
	 * @return 		Root of new tree.
	 */
	AcmBranch *acm_parse_buffer( const char *buf, const char *file );

	/**
	 * For troubleshooting. Prints out the tree based on the given branch.
	 *
	 * @param self 	Pointer to the root of the tree you want to print.
	 * @param index Should be passed as 0 - this is to track the depth into the tree.
	 */
	void acm_print_tree( AcmBranch *self, int index );

#define ACM_ITERATE_BRANCH( LIST, ITR ) for ( AcmBranch * ( ITR ) = acm_get_first_child( LIST ); \
	                                          ( ITR ) != nullptr;                                \
	                                          ( ITR ) = acm_get_next_child( ( ITR ) ) )

#if defined( __cplusplus )
}
#endif
