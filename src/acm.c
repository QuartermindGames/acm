// SPDX-License-Identifier: MIT
// Ape Config Markup
// Copyright © 2020-2025 Mark E Sowden <hogsy@oldtimes-software.com>

#include <stdarg.h>
#include <stdio.h>

#include "acm_private.h"

#include <ctype.h>
#include <inttypes.h>

#define ACM_FORMAT_UTF8_HEADER "node.utf8"

#define ACM_FORMAT_BINARY_HEADER   "node.bin\n" // original format w/ no versioning support (defaults to 1)
#define ACM_FORMAT_BINARY_HEADER_2 "node.binx\n"// new format w/ versioning support
#define ACM_FORMAT_BINARY_VERSION  2

static const char *string_for_property_type( AcmPropertyType propertyType )
{
	const char *propToStr[ ACM_MAX_PROPERTY_TYPES ] = {
	        // Special types
	        [ACM_PROPERTY_TYPE_OBJECT] = "object",
	        [ACM_PROPERTY_TYPE_STRING] = "string",
	        [ACM_PROPERTY_TYPE_BOOL]   = "bool",
	        [ACM_PROPERTY_TYPE_ARRAY]  = "array",
	        // Generic types
	        [ND_PROPERTY_INT8]          = "int8",
	        [ND_PROPERTY_INT16]         = "int16",
	        [ND_PROPERTY_INT32]         = "int32",
	        [ND_PROPERTY_INT64]         = "int64",
	        [ND_PROPERTY_UI8]           = "uint8",
	        [ND_PROPERTY_UI16]          = "uint16",
	        [ND_PROPERTY_UI32]          = "uint32",
	        [ND_PROPERTY_UI64]          = "uint64",
	        [ACM_PROPERTY_TYPE_FLOAT16] = "float16",
	        [ACM_PROPERTY_TYPE_FLOAT32] = "float",
	        [ACM_PROPERTY_TYPE_FLOAT64] = "float64",
	};

	if ( propertyType == ACM_PROPERTY_TYPE_INVALID )
	{
		return "invalid";
	}

	return propToStr[ propertyType ];
}

static char         nlErrorMsg[ 4096 ];
static AcmErrorCode nlErrorType = ND_ERROR_SUCCESS;
static void         clear_error_message( void )
{
	*nlErrorMsg = '\0';
	nlErrorType = ND_ERROR_SUCCESS;
}

static void set_error_message( AcmErrorCode type, const char *msg, ... )
{
	clear_error_message();

	nlErrorType = type;

	va_list args;
	va_start( args, msg );
	vsnprintf( nlErrorMsg, sizeof( nlErrorMsg ), msg, args );
	Warning( "NLERR: %s\n", nlErrorMsg );

	va_end( args );
}

const char  *acm_get_error_message( void ) { return nlErrorMsg; }
AcmErrorCode acm_get_error( void ) { return nlErrorType; }

static AcmString *alloc_var_string( const char *string, AcmString *dst )
{
	dst->bufSize = ( uint16_t ) strlen( string ) + 1;

	dst->buf = ACM_NEW_( char, dst->bufSize );
	if ( dst->buf == NULL )
	{
		set_error_message( NL_ERROR_MEM_ALLOC, "failed to allocate memory for variable string" );
		return NULL;
	}

	strcpy( dst->buf, string );

	return dst;
}

unsigned int acm_get_num_of_children( const AcmBranch *self )
{
	return self->numChildren;
}

AcmBranch *acm_get_first_child( AcmBranch *self )
{
	return self->children.start;
}

AcmBranch *acm_get_next_child( AcmBranch *node )
{
	return node->next;
}

AcmBranch *acm_get_child_by_name( AcmBranch *self, const char *name )
{
	if ( self->type != ACM_PROPERTY_TYPE_OBJECT )
	{
		set_error_message( ND_ERROR_INVALID_TYPE, "attempted to get child from an invalid node type!\n" );
		return NULL;
	}

	AcmBranch *child = acm_get_first_child( self );
	while ( child != NULL )
	{
		if ( strcmp( name, child->name.buf ) == 0 )
		{
			return child;
		}

		child = acm_get_next_child( child );
	}

	return NULL;
}

static const AcmString *get_value_by_name( AcmBranch *root, const char *name )
{
	const AcmBranch *field = acm_get_child_by_name( root, name );
	if ( field == NULL )
	{
		return NULL;
	}

	return &field->data;
}

AcmBranch *acm_get_parent( AcmBranch *self )
{
	return self->parent;
}

const char *acm_branch_get_name( const AcmBranch *self )
{
	return self->name.buf;
}

AcmPropertyType acm_branch_get_type( const AcmBranch *self )
{
	return self->type;
}

const char *acm_branch_get_value( const AcmBranch *self, uint16_t *size )
{
	if ( size != NULL )
	{
		*size = self->data.bufSize;
	}
	return self->data.buf;
}

AcmErrorCode acm_branch_get_string( const AcmBranch *self, char *dest, size_t length )
{
	//TODO: why isn't this just returning the buf!?
	//		if we can resolve this, it'll negate the need for get_value
	if ( self->type != ACM_PROPERTY_TYPE_STRING ) return ND_ERROR_INVALID_TYPE;
	snprintf( dest, length, "%s", self->data.buf );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_bool( const AcmBranch *self, bool *dest )
{
	if ( self->type != ACM_PROPERTY_TYPE_BOOL ) return ND_ERROR_INVALID_TYPE;

	if ( ( strcmp( self->data.buf, "true" ) == 0 ) || ( self->data.buf[ 0 ] == '1' && self->data.buf[ 1 ] == '\0' ) )
	{
		*dest = true;
		return ND_ERROR_SUCCESS;
	}
	if ( ( strcmp( self->data.buf, "false" ) == 0 ) || ( self->data.buf[ 0 ] == '0' && self->data.buf[ 1 ] == '\0' ) )
	{
		*dest = false;
		return ND_ERROR_SUCCESS;
	}

	set_error_message( ND_ERROR_INVALID_ARGUMENT, "invalid data passed from var" );
	return ND_ERROR_INVALID_ARGUMENT;
}

AcmErrorCode acm_branch_get_float16( const AcmBranch *self, _Float16 *dest )
{
	if ( self->type != ACM_PROPERTY_TYPE_FLOAT16 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtof( self->data.buf, NULL );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_float32( const AcmBranch *self, float *dest )
{
	if ( self->type != ACM_PROPERTY_TYPE_FLOAT32 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtof( self->data.buf, NULL );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_float64( const AcmBranch *self, double *dest )
{
	if ( self->type != ACM_PROPERTY_TYPE_FLOAT64 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtod( self->data.buf, NULL );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_int8( const AcmBranch *self, int8_t *dest )
{
	if ( self->type != ND_PROPERTY_INT8 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( int8_t ) strtol( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_int16( const AcmBranch *self, int16_t *dest )
{
	if ( self->type != ND_PROPERTY_INT16 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( int16_t ) strtol( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_int32( const AcmBranch *self, int32_t *dest )
{
	if ( self->type != ND_PROPERTY_INT32 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( int32_t ) strtol( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_int64( const AcmBranch *self, int64_t *dest )
{
	if ( self->type != ND_PROPERTY_INT64 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtoll( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_uint8( const AcmBranch *self, uint8_t *dest )
{
	if ( self->type != ND_PROPERTY_UI8 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( uint8_t ) strtoul( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_uint16( const AcmBranch *self, uint16_t *dest )
{
	if ( self->type != ND_PROPERTY_UI16 ) return ND_ERROR_INVALID_TYPE;
	*dest = ( uint16_t ) strtoul( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_uint32( const AcmBranch *self, uint32_t *dest )
{
	if ( self->type != ND_PROPERTY_UI32 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtoul( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_uint64( const AcmBranch *self, uint64_t *dest )
{
	if ( self->type != ND_PROPERTY_UI64 ) return ND_ERROR_INVALID_TYPE;
	*dest = strtoull( self->data.buf, NULL, 10 );
	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_string_array( AcmBranch *self, char **buf, unsigned int numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ACM_PROPERTY_TYPE_STRING )
	{
		return ND_ERROR_INVALID_TYPE;
	}

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
		{
			return ND_ERROR_INVALID_ELEMENTS;
		}

		buf[ i ] = ACM_NEW_( char, strlen( child->data.buf ) + 1 );
		strcpy( buf[ i ], child->data.buf );

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_bool_array( AcmBranch *self, bool *buf, unsigned int numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ACM_PROPERTY_TYPE_FLOAT64 )
		return ND_ERROR_INVALID_TYPE;

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		AcmErrorCode errorCode = acm_branch_get_bool( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_int8_array( AcmBranch *self, int8_t *buf, unsigned int numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ND_PROPERTY_INT8 )
		return ND_ERROR_INVALID_TYPE;

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		AcmErrorCode errorCode = acm_branch_get_int8( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_int16_array( AcmBranch *self, int16_t *buf, unsigned int numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ND_PROPERTY_INT16 )
		return ND_ERROR_INVALID_TYPE;

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		AcmErrorCode errorCode = acm_branch_get_int16( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_int32_array( AcmBranch *self, int32_t *buf, unsigned int numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ND_PROPERTY_INT32 )
		return ND_ERROR_INVALID_TYPE;

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		AcmErrorCode errorCode = acm_branch_get_int32( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_uint32_array( AcmBranch *self, uint32_t *buf, unsigned int numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ND_PROPERTY_UI32 )
		return ND_ERROR_INVALID_TYPE;

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		AcmErrorCode errorCode = acm_branch_get_uint32( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_float32_array( AcmBranch *self, float *buf, unsigned int numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ACM_PROPERTY_TYPE_FLOAT32 )
		return ND_ERROR_INVALID_TYPE;

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
			return ND_ERROR_INVALID_ELEMENTS;

		AcmErrorCode errorCode = acm_branch_get_float32( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
		{
			return errorCode;
		}

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

AcmErrorCode acm_branch_get_float64_array( AcmBranch *self, double *buf, unsigned int numElements )
{
	if ( self->type != ACM_PROPERTY_TYPE_ARRAY || self->childType != ACM_PROPERTY_TYPE_FLOAT64 )
	{
		return ND_ERROR_INVALID_TYPE;
	}

	AcmBranch *child = acm_get_first_child( self );
	for ( unsigned int i = 0; i < numElements; ++i )
	{
		if ( child == NULL )
		{
			return ND_ERROR_INVALID_ELEMENTS;
		}

		AcmErrorCode errorCode = acm_branch_get_float64( child, &buf[ i ] );
		if ( errorCode != ND_ERROR_SUCCESS )
			return errorCode;

		child = acm_get_next_child( child );
	}

	return ND_ERROR_SUCCESS;
}

/******************************************/
/** Get: ByName **/

bool acm_get_bool( AcmBranch *root, const char *name, bool fallback )
{
	const AcmBranch *child = acm_get_child_by_name( root, name );
	if ( child == NULL )
	{
		return fallback;
	}

	bool out;
	if ( acm_branch_get_bool( child, &out ) != ND_ERROR_SUCCESS )
	{
		return fallback;
	}

	return out;
}

const char *acm_get_string( AcmBranch *node, const char *name, const char *fallback )
{
	/* todo: warning on fail */
	const AcmString *var = get_value_by_name( node, name );
	return ( var != NULL ) ? var->buf : fallback;
}

float acm_get_f32( AcmBranch *node, const char *name, float fallback )
{
	return ( float ) acm_get_f64( node, name, fallback );
}

float *acm_get_array_f32( AcmBranch *branch, const char *name, float *destination, uint32_t numElements )
{
	AcmBranch *child = acm_get_child_by_name( branch, name );
	if ( child == NULL )
	{
		return NULL;
	}

	if ( acm_branch_get_float32_array( child, destination, numElements ) != ND_ERROR_SUCCESS )
	{
		return NULL;
	}

	return destination;
}

double acm_get_f64( AcmBranch *node, const char *name, double fallback )
{
	/* todo: warning on fail */
	const AcmString *var = get_value_by_name( node, name );
	return var != NULL ? strtod( var->buf, NULL ) : fallback;
}

intmax_t acm_get_int( AcmBranch *root, const char *name, intmax_t fallback )
{
	const AcmString *var = get_value_by_name( root, name );
	return ( var != NULL ) ? strtoll( var->buf, NULL, 10 ) : fallback;
}

uintmax_t acm_get_uint( AcmBranch *root, const char *name, uintmax_t fallback )
{
	const AcmString *var = get_value_by_name( root, name );
	return ( var != NULL ) ? strtoull( var->buf, NULL, 10 ) : fallback;
}

int16_t *acm_get_array_i16( AcmBranch *branch, const char *name, int16_t *destination, unsigned int numElements )
{
	AcmBranch *child = acm_get_child_by_name( branch, name );
	if ( child == NULL )
	{
		return NULL;
	}

	if ( acm_branch_get_int16_array( child, destination, numElements ) != ND_ERROR_SUCCESS )
	{
		return NULL;
	}

	return destination;
}

static int acm_strcasecmp( const char *s1, const char *s2 )
{
	const unsigned char *us1 = ( const unsigned char * ) s1;
	const unsigned char *us2 = ( const unsigned char * ) s2;

	while ( tolower( *us1 ) == tolower( *us2++ ) )
	{
		if ( *us1++ == '\0' )
		{
			return 0;
		}
	}

	return tolower( *us1 ) - tolower( *--us2 );
}

AcmBranch *acm_linear_lookup( AcmBranch *root, const char *name )
{
	if ( root->name.buf != NULL && acm_strcasecmp( root->name.buf, name ) == 0 )
	{
		return root;
	}

	AcmBranch *child = root->children.start;
	while ( child != NULL )
	{
		AcmBranch *result = acm_linear_lookup( child, name );
		if ( result != NULL )
		{
			return result;
		}

		child = child->next;
	}

	return NULL;
}

/******************************************/

static void attach_branch( AcmBranch *self, AcmBranch *parent )
{
	if ( parent->children.start == NULL )
	{
		parent->children.start = self;
	}

	self->prev = parent->children.end;
	if ( parent->children.end != NULL )
	{
		parent->children.end->next = self;
	}
	parent->children.end = self;
	self->next           = NULL;
	self->parent         = parent;

	parent->numChildren++;
}

AcmBranch *acm_push_new_branch( AcmBranch *parent, const char *name, AcmPropertyType propertyType, AcmPropertyType childType )
{
	/* arrays are special cases */
	if ( parent != NULL && parent->type == ACM_PROPERTY_TYPE_ARRAY && propertyType != parent->childType )
	{
		set_error_message( ND_ERROR_INVALID_TYPE, "attempted to add invalid type (%s)", string_for_property_type( propertyType ) );
		return NULL;
	}

	AcmBranch *node = ACM_NEW( AcmBranch );

	/* assign the node name, if provided */
	if ( ( parent == NULL || parent->type != ACM_PROPERTY_TYPE_ARRAY ) && name != NULL )
	{
		alloc_var_string( name, &node->name );
	}

	node->type      = propertyType;
	node->childType = childType;// only matters for array

	/* if parent is provided, this is treated as a child of that node */
	if ( parent != NULL )
	{
		attach_branch( node, parent );
	}

	return node;
}

AcmBranch *acm_push_variable_( AcmBranch *parent, const char *name, const char *value, AcmPropertyType type )
{
	AcmBranch *branch = acm_push_new_branch( parent, name, type, ACM_PROPERTY_TYPE_INVALID );
	if ( branch == NULL )
	{
		return NULL;
	}

	alloc_var_string( value, &branch->data );
	return branch;
}

AcmBranch *acm_push_branch( AcmBranch *parent, AcmBranch *child )
{
	AcmBranch *branch = acm_copy_branch( child );
	attach_branch( branch, parent );
	return branch;
}

AcmBranch *acm_push_object( AcmBranch *node, const char *name )
{
	return acm_push_new_branch( node, name, ACM_PROPERTY_TYPE_OBJECT, ACM_PROPERTY_TYPE_INVALID );
}

AcmBranch *acm_push_string( AcmBranch *parent, const char *name, const char *var, bool conditional )
{
	if ( conditional && *var == '\0' )
	{
		return NULL;
	}

	return acm_push_variable_( parent, name, var, ACM_PROPERTY_TYPE_STRING );
}

AcmBranch *acm_push_array_string( AcmBranch *parent, const char *name, const char **array, unsigned int numElements )
{
	AcmBranch *node = acm_push_new_branch( parent, name, ACM_PROPERTY_TYPE_ARRAY, ACM_PROPERTY_TYPE_STRING );
	if ( node != NULL )
	{
		for ( unsigned int i = 0; i < numElements; ++i )
		{
			acm_push_string( node, NULL, array[ i ], false );
		}
	}
	return node;
}

AcmBranch *acm_push_bool( AcmBranch *parent, const char *name, bool var )
{
	return acm_push_variable_( parent, name, var ? "true" : "false", ACM_PROPERTY_TYPE_BOOL );
}

AcmBranch *acm_push_i8( AcmBranch *parent, const char *name, int8_t var )
{
	char buf[ 8 ];
	snprintf( buf, sizeof( buf ), "%" PRId8, var );
	return acm_push_variable_( parent, name, buf, ND_PROPERTY_INT8 );
}

AcmBranch *acm_push_i16( AcmBranch *parent, const char *name, int16_t var )
{
	char buf[ 32 ];
	snprintf( buf, sizeof( buf ), "%" PRId16, var );
	return acm_push_variable_( parent, name, buf, ND_PROPERTY_INT16 );
}

AcmBranch *acm_push_ui16( AcmBranch *parent, const char *name, uint16_t var )
{
	char buf[ 32 ];
	snprintf( buf, sizeof( buf ), "%" PRIu16, var );
	return acm_push_variable_( parent, name, buf, ND_PROPERTY_UI16 );
}

AcmBranch *acm_push_i32( AcmBranch *parent, const char *name, int32_t var )
{
	char buf[ 32 ];
	snprintf( buf, sizeof( buf ), "%" PRId32, var );
	return acm_push_variable_( parent, name, buf, ND_PROPERTY_INT32 );
}

AcmBranch *acm_push_ui32( AcmBranch *parent, const char *name, uint32_t var )
{
	char buf[ 32 ];
	snprintf( buf, sizeof( buf ), "%" PRIu32, var );
	return acm_push_variable_( parent, name, buf, ND_PROPERTY_UI32 );
}

AcmBranch *acm_push_f16( AcmBranch *parent, const char *name, _Float16 var )
{
	char buf[ 32 ];
	snprintf( buf, sizeof( buf ), "%f", ( double ) var );
	return acm_push_variable_( parent, name, buf, ACM_PROPERTY_TYPE_FLOAT16 );
}

AcmBranch *acm_push_f32( AcmBranch *parent, const char *name, float var )
{
	char buf[ 32 ];
	snprintf( buf, sizeof( buf ), "%f", var );
	return acm_push_variable_( parent, name, buf, ACM_PROPERTY_TYPE_FLOAT32 );
}

AcmBranch *acm_push_f64( AcmBranch *parent, const char *name, double var )
{
	char buf[ 32 ];
	snprintf( buf, sizeof( buf ), "%lf", var );
	return acm_push_variable_( parent, name, buf, ACM_PROPERTY_TYPE_FLOAT64 );
}

AcmBranch *acm_push_array_i16( AcmBranch *root, const char *name, const int16_t *array, unsigned int numElements )
{
	AcmBranch *node = acm_push_new_branch( root, name, ACM_PROPERTY_TYPE_ARRAY, ND_PROPERTY_INT16 );
	if ( node != NULL )
	{
		for ( unsigned int i = 0; i < numElements; ++i )
		{
			acm_push_i16( node, NULL, array[ i ] );
		}
	}
	return node;
}

AcmBranch *acm_push_array_i32( AcmBranch *parent, const char *name, const int32_t *array, unsigned int numElements )
{
	AcmBranch *node = acm_push_new_branch( parent, name, ACM_PROPERTY_TYPE_ARRAY, ND_PROPERTY_INT32 );
	if ( node != NULL )
	{
		for ( unsigned int i = 0; i < numElements; ++i )
		{
			acm_push_i32( node, NULL, array[ i ] );
		}
	}
	return node;
}

AcmBranch *acm_push_array_ui32( AcmBranch *parent, const char *name, const uint32_t *array, unsigned int numElements )
{
	AcmBranch *node = acm_push_new_branch( parent, name, ACM_PROPERTY_TYPE_ARRAY, ND_PROPERTY_UI32 );
	if ( node != NULL )
	{
		for ( unsigned int i = 0; i < numElements; ++i )
		{
			acm_push_ui32( node, NULL, array[ i ] );
		}
	}
	return node;
}

AcmBranch *acm_push_array_f16( AcmBranch *parent, const char *name, const _Float16 *array, unsigned int numElements )
{
	AcmBranch *node = acm_push_new_branch( parent, name, ACM_PROPERTY_TYPE_ARRAY, ACM_PROPERTY_TYPE_FLOAT16 );
	if ( node != NULL )
	{
		for ( unsigned int i = 0; i < numElements; ++i )
		{
			acm_push_f16( node, NULL, array[ i ] );
		}
	}
	return node;
}

AcmBranch *acm_push_array_f32( AcmBranch *parent, const char *name, const float *array, unsigned int numElements )
{
	AcmBranch *node = acm_push_new_branch( parent, name, ACM_PROPERTY_TYPE_ARRAY, ACM_PROPERTY_TYPE_FLOAT32 );
	if ( node != NULL )
	{
		for ( unsigned int i = 0; i < numElements; ++i )
		{
			acm_push_f32( node, NULL, array[ i ] );
		}
	}
	return node;
}

bool acm_set_variable( AcmBranch *root, const char *name, const char *value, AcmPropertyType type, bool createOnFail )
{
	AcmBranch *child = acm_get_child_by_name( root, name );
	if ( child == NULL )
	{
		if ( !createOnFail )
		{
			return false;
		}

		return acm_push_variable_( root, name, value, type ) != NULL;
	}

	if ( child->type != type )
	{
		set_error_message( ND_ERROR_INVALID_TYPE, "attempted to set variable (%s) to invalid type (%s)", name, string_for_property_type( type ) );
		return false;
	}

	size_t length = strlen( value ) + 1;
	if ( length > child->data.bufSize )
	{
		void *p = ACM_REALLOC( child->data.buf, char, length );
		if ( p == NULL )
		{
			set_error_message( NL_ERROR_MEM_ALLOC, "failed to allocate memory for variable (%s)", name );
			return false;
		}

		child->data.buf     = p;
		child->data.bufSize = length;
	}

	snprintf( child->data.buf, child->data.bufSize, "%s", value );

	return true;
}

AcmBranch *acm_push_array_object( AcmBranch *parent, const char *name )
{
	return acm_push_new_branch( parent, name, ACM_PROPERTY_TYPE_ARRAY, ACM_PROPERTY_TYPE_OBJECT );
}

static AcmString *copy_var_string( const AcmString *src, AcmString *dst )
{
	dst->bufSize = src->bufSize;

	dst->buf = ACM_NEW_( char, src->bufSize );
	if ( dst->buf == NULL )
	{
		set_error_message( NL_ERROR_MEM_ALLOC, "failed to allocate memory for variable string" );
		return NULL;
	}

	strcpy( dst->buf, src->buf );

	return dst;
}

/**
 * Copies the given node list.
 */
AcmBranch *acm_copy_branch( AcmBranch *node )
{
	AcmBranch *newNode = ACM_NEW( AcmBranch );
	newNode->type      = node->type;
	newNode->childType = node->childType;
	copy_var_string( &node->data, &newNode->data );
	copy_var_string( &node->name, &newNode->name );
	// Not setting the parent is intentional here, since we likely don't want that link

	AcmBranch *child = acm_get_first_child( node );
	while ( child != NULL )
	{
		AcmBranch *newChild = acm_copy_branch( child );
		attach_branch( newChild, newNode );
		child = acm_get_next_child( child );
	}

	return newNode;
}

void acm_branch_destroy( AcmBranch *node )
{
	if ( node == NULL )
	{
		return;
	}

	ACM_DELETE( node->name.buf );
	ACM_DELETE( node->data.buf );

	/* if it's an object/array, we'll need to clean up all it's children */
	if ( node->type == ACM_PROPERTY_TYPE_OBJECT || node->type == ACM_PROPERTY_TYPE_ARRAY )
	{
		AcmBranch *child = acm_get_first_child( node );
		while ( child != NULL )
		{
			AcmBranch *nextChild = acm_get_next_child( child );
			acm_branch_destroy( child );
			child = nextChild;
		}
	}

	if ( node->parent != NULL )
	{
		if ( node->prev != NULL )
		{
			node->prev->next = node->next;
		}
		if ( node->next != NULL )
		{
			node->next->prev = node->prev;
		}

		if ( node == node->parent->children.start )
		{
			node->parent->children.start = node->next;
		}
		if ( node == node->parent->children.end )
		{
			node->parent->children.end = node->prev;
		}

		node->parent->numChildren--;
	}

	ACM_DELETE( node );
}

/******************************************/
/** Deserialisation **/

static const void *read_buf( const void **buf, size_t *bufSize, size_t elementSize )
{
	if ( elementSize == 0 || *bufSize < elementSize )
	{
		return NULL;
	}

	const void *p = *buf;
	*buf          = ( const char * ) ( *buf ) + elementSize;
	*bufSize -= elementSize;
	return p;
}

static char *read_string( const void **buf, size_t *bufSize, uint16_t size )
{
	const char *src = read_buf( buf, bufSize, size );
	if ( src == NULL )
	{
		return NULL;
	}

	char *string = ACM_NEW_( char, size + 1 );
	strcpy( string, src );
	return string;
}

static char *deserialize_string_var( const void **buf, size_t *bufSize, uint16_t *length )
{
	const uint16_t *l = read_buf( buf, bufSize, sizeof( uint16_t ) );
	if ( l == NULL )
	{
		return NULL;
	}

	*length = *l;
	if ( *length == 0 )
	{
		return NULL;
	}

	return read_string( buf, bufSize, *length );
}

static AcmBranch *deserialize_binary_node( const void **buf, size_t *bufSize, AcmBranch *parent, unsigned int version )
{
	// attempt to fetch the name, keeping in mind that not
	// all nodes necessarily have a name
	AcmString name;
	name.buf = deserialize_string_var( buf, bufSize, &name.bufSize );

	const int8_t *type = read_buf( buf, bufSize, sizeof( int8_t ) );
	if ( type == NULL )
	{
		Warning( "Failed to read property type for node (%s)!\n", name.buf ? name.buf : "unnamed" );
		ACM_DELETE( name.buf );
		return NULL;
	}

	if ( *type == ACM_PROPERTY_TYPE_INVALID || *type >= ACM_MAX_PROPERTY_TYPES )
	{
		Warning( "Invalid property type (%u) for node (%s)!\n", *type, name.buf ? name.buf : "unnamed" );
		ACM_DELETE( name.buf );
		return NULL;
	}

	AcmBranch *node = acm_push_new_branch( parent, NULL, *type, ACM_PROPERTY_TYPE_INVALID );
	if ( node == NULL )
	{
		ACM_DELETE( name.buf );
		return NULL;
	}

	// node now takes ownership of name
	node->name = name;

	if ( node->type == ACM_PROPERTY_TYPE_ARRAY )
	{
		const int8_t *childType = read_buf( buf, bufSize, sizeof( int8_t ) );
		if ( childType == NULL )
		{
			Warning( "Failed to fetch child type for node!\n" );
			acm_branch_destroy( node );
			return NULL;
		}

		if ( *childType == ACM_PROPERTY_TYPE_INVALID || *childType >= ACM_MAX_PROPERTY_TYPES )
		{
			Warning( "Invalid child property type (%u) for node (%s)!\n", *childType, name.buf ? name.buf : "unnamed" );
			acm_branch_destroy( node );
			return NULL;
		}

		node->childType = ( AcmPropertyType ) *childType;
	}

	// get the expected size depending on the type
	unsigned int typeSize;
	switch ( node->type )
	{
		default:
			typeSize = 0;
			break;
		case ACM_PROPERTY_TYPE_FLOAT32:
			typeSize = sizeof( float );
			break;
		case ACM_PROPERTY_TYPE_FLOAT64:
			typeSize = sizeof( double );
			break;
		case ACM_PROPERTY_TYPE_BOOL:
		case ND_PROPERTY_INT8:
		case ND_PROPERTY_UI8:
			typeSize = sizeof( uint8_t );
			break;
		case ACM_PROPERTY_TYPE_FLOAT16:
		case ACM_PROPERTY_TYPE_STRING:
		case ND_PROPERTY_INT16:
		case ND_PROPERTY_UI16:
			typeSize = sizeof( uint16_t );
			break;
		case ACM_PROPERTY_TYPE_ARRAY:
		case ACM_PROPERTY_TYPE_OBJECT:
		case ND_PROPERTY_INT32:
		case ND_PROPERTY_UI32:
			typeSize = sizeof( uint32_t );
			break;
		case ND_PROPERTY_INT64:
		case ND_PROPERTY_UI64:
			typeSize = sizeof( uint64_t );
			break;
	}

	if ( typeSize == 0 )
	{
		Warning( "Unexpected type size for node; likely an unhandled type (%u)!\n", node->type );
		acm_branch_destroy( node );
		return NULL;
	}

	const void *data = read_buf( buf, bufSize, typeSize );
	if ( data == NULL )
	{
		Warning( "Failed to fetch initial data value for type (%u)!\n", node->type );
		acm_branch_destroy( node );
		return NULL;
	}

	switch ( node->type )
	{
		default:
			Warning( "Encountered unhandled node type: %d!\n", node->type );
			acm_branch_destroy( node );
			return NULL;
		case ACM_PROPERTY_TYPE_ARRAY:
		case ACM_PROPERTY_TYPE_OBJECT:
		{
			unsigned int numChildren = *( ( uint32_t * ) data );
			for ( unsigned int i = 0; i < numChildren; ++i )
			{
				if ( deserialize_binary_node( buf, bufSize, node, version ) == NULL )
				{
					break;
				}
			}
			break;
		}
		case ACM_PROPERTY_TYPE_STRING:
		{
			node->data.bufSize = *( ( uint16_t * ) data );
			node->data.buf     = read_string( buf, bufSize, node->data.bufSize );
			break;
		}
		case ACM_PROPERTY_TYPE_BOOL:
		{
			alloc_var_string( *( ( bool * ) data ) ? "true" : "false", &node->data );
			break;
		}
		case ACM_PROPERTY_TYPE_FLOAT16:
		{
			char str[ 32 ];
			snprintf( str, sizeof( str ), "%f", ( double ) *( _Float16 * ) data );
			alloc_var_string( str, &node->data );
			break;
		}
		case ACM_PROPERTY_TYPE_FLOAT32:
		{
			char str[ 32 ];
			snprintf( str, sizeof( str ), "%f", *( float * ) data );
			alloc_var_string( str, &node->data );
			break;
		}
		case ACM_PROPERTY_TYPE_FLOAT64:
		{
			char str[ 32 ];
			snprintf( str, sizeof( str ), "%lf", *( double * ) data );
			alloc_var_string( str, &node->data );
			break;
		}
		case ND_PROPERTY_UI8:
		{
			char str[ 32 ];
			snprintf( str, sizeof( str ), "%" PRIu8, *( uint8_t * ) data );
			alloc_var_string( str, &node->data );
			break;
		}
		case ND_PROPERTY_INT8:
		{
			char str[ 32 ];
			snprintf( str, sizeof( str ), "%" PRId8, *( int8_t * ) data );
			alloc_var_string( str, &node->data );
			break;
		}
		case ND_PROPERTY_UI16:
		{
			char str[ 32 ];
			snprintf( str, sizeof( str ), "%" PRIu16, *( uint16_t * ) data );
			alloc_var_string( str, &node->data );
			// slapped on fix for a bug with serialisation in older versions
			if ( version < 2 )
			{
				read_buf( buf, bufSize, sizeof( uint32_t ) );
			}
			break;
		}
		case ND_PROPERTY_INT16:
		{
			char str[ 32 ];
			snprintf( str, sizeof( str ), "%" PRId16, *( int16_t * ) data );
			alloc_var_string( str, &node->data );
			// slapped on fix for a bug with serialisation in older versions
			if ( version < 2 )
			{
				read_buf( buf, bufSize, sizeof( uint32_t ) );
			}
			break;
		}
		case ND_PROPERTY_UI32:
		{
			char str[ 32 ];
			snprintf( str, sizeof( str ), "%" PRIu32, *( uint32_t * ) data );
			alloc_var_string( str, &node->data );
			break;
		}
		case ND_PROPERTY_INT32:
		{
			char str[ 32 ];
			snprintf( str, sizeof( str ), "%" PRId32, *( int32_t * ) data );
			alloc_var_string( str, &node->data );
			break;
		}
		case ND_PROPERTY_UI64:
		{
			char str[ 32 ];
			snprintf( str, sizeof( str ), "%" PRIu64, *( uint64_t * ) data );
			alloc_var_string( str, &node->data );
			break;
		}
		case ND_PROPERTY_INT64:
		{
			char str[ 32 ];
			snprintf( str, sizeof( str ), "%" PRId64, *( int64_t * ) data );
			alloc_var_string( str, &node->data );
			break;
		}
	}

	return node;
}

static AcmFileType parse_node_file_type( const void *buf, uint32_t *version, unsigned int *headerSize )
{
	*headerSize = strlen( ACM_FORMAT_UTF8_HEADER );
	if ( strncmp( buf, ACM_FORMAT_UTF8_HEADER, *headerSize ) == 0 )
	{
		*version = 1;
		return ACM_FILE_TYPE_UTF8;
	}

	*headerSize = strlen( ACM_FORMAT_BINARY_HEADER_2 );
	if ( strncmp( buf, ACM_FORMAT_BINARY_HEADER_2, *headerSize ) == 0 )
	{
		*version = *( uint32_t * ) ( buf + *headerSize );// inc + 1, because there's a new line after identifier
		if ( *version == 0 || *version > ACM_FORMAT_BINARY_VERSION )
		{
			set_error_message( ND_ERROR_IO_READ, "invalid binary node format (%u == 0 || %u > %u)", *version, *version, ACM_FORMAT_BINARY_VERSION );
			return ACM_FILE_TYPE_INVALID;
		}

		*headerSize = *headerSize + sizeof( uint32_t );
		return ACM_FILE_TYPE_BINARY;
	}

	*headerSize = strlen( ACM_FORMAT_BINARY_HEADER );
	if ( strncmp( buf, ACM_FORMAT_BINARY_HEADER, *headerSize ) == 0 )
	{
		*version = 1;
		return ACM_FILE_TYPE_BINARY;
	}

	set_error_message( ND_ERROR_IO_READ, "unknown file type" );
	return ACM_FILE_TYPE_INVALID;
}

AcmBranch *acm_load_from_memory( const void *buf, size_t bufSize, const char *objectType, const char *source )
{
	AcmBranch *root = NULL;

	unsigned int headerSize;
	unsigned int version;
	AcmFileType  fileType = parse_node_file_type( buf, &version, &headerSize );
	if ( fileType == ACM_FILE_TYPE_BINARY )
	{
		const void *p = buf;
		read_buf( &p, &bufSize, headerSize );
		root = deserialize_binary_node( &p, &bufSize, NULL, version );
	}
	else if ( fileType == ACM_FILE_TYPE_UTF8 )
	{
		root = acm_parse_buffer( ( const char * ) buf + headerSize, source );
	}
	else
	{
		Warning( "Invalid node file type: %d\n", fileType );
	}

	if ( root != NULL && objectType != NULL )
	{
		const char *rootName = acm_branch_get_name( root );
		if ( strcmp( rootName, objectType ) != 0 )
		{
			/* destroy the tree */
			acm_branch_destroy( root );

			Warning( "Invalid \"%s\" file, expected \"%s\" but got \"%s\"!\n", objectType, objectType, rootName );
			return NULL;
		}
	}

	return root;
}

AcmBranch *acm_load_file( const char *path, const char *objectType )
{
	clear_error_message();

	FILE *file = fopen( path, "rb" );
	if ( file == NULL )
	{
		snprintf( nlErrorMsg, sizeof( nlErrorMsg ), "failed to open file (%s)", path );
		nlErrorType = ND_ERROR_IO_READ;
		return NULL;
	}

	// determine the size of the file
	fseek( file, 0, SEEK_END );
	size_t size = ftell( file );
	rewind( file );

	AcmBranch *root = NULL;

	// now read it into memory
	uint8_t *buf = ACM_NEW_( uint8_t, size + 1 );
	if ( buf != NULL )
	{
		fread( buf, sizeof( uint8_t ), size, file );

		// finally hand it over to our parser
		root = acm_load_from_memory( buf, size, objectType, path );

		ACM_DELETE( buf );
	}

	fclose( file );

	return root;
}

/******************************************/
/** Serialisation **/

static unsigned int sDepth; /* serialisation depth */

static void write_line( FILE *file, const char *string, bool tabify )
{
	if ( tabify )
	{
		for ( unsigned int i = 0; i < sDepth; ++i )
		{
			fputc( '\t', file );
		}
	}

	if ( string == NULL )
	{
		return;
	}

	fprintf( file, "%s", string );
}

static void serialize_string_var( const AcmString *string, AcmFileType fileType, FILE *file )
{
	size_t length = string->buf == NULL ? 0 : strlen( string->buf );
	if ( fileType == ACM_FILE_TYPE_BINARY )
	{
		if ( length > 0 )
		{
			length++;// null terminator
		}

		fwrite( &length, sizeof( uint16_t ), 1, file );
		fwrite( string->buf, sizeof( char ), length, file );
		return;
	}

	/* allow nameless nodes, used for arrays */
	if ( length == 0 )
	{
		return;
	}

	bool        encloseString = false;
	const char *c             = string->buf;
	if ( *c == '\0' )
	{
		/* enclose an empty string!!! */
		encloseString = true;
	}
	else
	{
		/* otherwise, check if there are any spaces */
		while ( *c != '\0' )
		{
			if ( *c == ' ' )
			{
				encloseString = true;
				break;
			}

			c++;
		}
	}

	if ( encloseString )
	{
		fprintf( file, "\"%s\" ", string->buf );
	}
	else
	{
		fprintf( file, "%s ", string->buf );
	}
}

static void serialize_node_tree( FILE *file, AcmBranch *root, AcmFileType fileType );
static void serialize_node( FILE *file, AcmBranch *node, AcmFileType fileType )
{
	if ( fileType == ACM_FILE_TYPE_UTF8 )
	{
		/* write out the line identifying this node */
		write_line( file, NULL, true );
		AcmBranch *parent = acm_get_parent( node );
		if ( parent == NULL || parent->type != ACM_PROPERTY_TYPE_ARRAY )
		{
			fprintf( file, "%s ", string_for_property_type( node->type ) );
			if ( node->type == ACM_PROPERTY_TYPE_ARRAY )
			{
				fprintf( file, "%s ", string_for_property_type( node->childType ) );
			}

			serialize_string_var( &node->name, fileType, file );
		}

		/* if this node has children, serialize all those */
		if ( node->type == ACM_PROPERTY_TYPE_OBJECT || node->type == ACM_PROPERTY_TYPE_ARRAY )
		{
			write_line( file, "{\n", ( parent != NULL && parent->type == ACM_PROPERTY_TYPE_ARRAY ) );
			sDepth++;
			serialize_node_tree( file, node, fileType );
			sDepth--;
			write_line( file, "}\n", true );
		}
		else
		{
			serialize_string_var( &node->data, fileType, file );
			fprintf( file, "\n" );
		}

		return;
	}

	serialize_string_var( &node->name, fileType, file );
	fwrite( &node->type, sizeof( int8_t ), 1, file );
	switch ( node->type )
	{
		default:
		{
			Warning( "Invalid node type: %u/n", node->type );
			//TODO: don't do this!!!
			abort();
		}
		case ACM_PROPERTY_TYPE_FLOAT16:
		{
			_Float16 v;
			acm_branch_get_float16( node, &v );
			fwrite( &v, sizeof( _Float16 ), 1, file );
			break;
		}
		case ACM_PROPERTY_TYPE_FLOAT32:
		{
			float v;
			acm_branch_get_float32( node, &v );
			fwrite( &v, sizeof( float ), 1, file );
			break;
		}
		case ACM_PROPERTY_TYPE_FLOAT64:
		{
			double v;
			acm_branch_get_float64( node, &v );
			fwrite( &v, sizeof( double ), 1, file );
			break;
		}
		case ND_PROPERTY_INT8:
		{
			int8_t v;
			acm_branch_get_int8( node, &v );
			fwrite( &v, sizeof( int8_t ), 1, file );
			break;
		}
		case ND_PROPERTY_INT16:
		{
			int16_t v;
			acm_branch_get_int16( node, &v );
			fwrite( &v, sizeof( int16_t ), 1, file );
			break;
		}
		case ND_PROPERTY_INT32:
		{
			int32_t v;
			acm_branch_get_int32( node, &v );
			fwrite( &v, sizeof( int32_t ), 1, file );
			break;
		}
		case ND_PROPERTY_INT64:
		{
			int64_t v;
			acm_branch_get_int64( node, &v );
			fwrite( &v, sizeof( int64_t ), 1, file );
			break;
		}
		case ND_PROPERTY_UI8:
		{
			uint8_t v;
			acm_branch_get_uint8( node, &v );
			fwrite( &v, sizeof( uint8_t ), 1, file );
			break;
		}
		case ND_PROPERTY_UI16:
		{
			uint16_t v;
			acm_branch_get_uint16( node, &v );
			fwrite( &v, sizeof( uint16_t ), 1, file );
			break;
		}
		case ND_PROPERTY_UI32:
		{
			uint32_t v;
			acm_branch_get_uint32( node, &v );
			fwrite( &v, sizeof( uint32_t ), 1, file );
			break;
		}
		case ND_PROPERTY_UI64:
		{
			uint64_t v;
			acm_branch_get_uint64( node, &v );
			fwrite( &v, sizeof( uint64_t ), 1, file );
			break;
		}
		case ACM_PROPERTY_TYPE_STRING:
		{
			serialize_string_var( &node->data, fileType, file );
			break;
		}
		case ACM_PROPERTY_TYPE_BOOL:
		{
			bool v;
			acm_branch_get_bool( node, &v );
			fwrite( &v, sizeof( uint8_t ), 1, file );
			break;
		}
		case ACM_PROPERTY_TYPE_ARRAY:
		{
			/* only extra component here is the child type */
			fwrite( &node->childType, sizeof( uint8_t ), 1, file );
		}
		case ACM_PROPERTY_TYPE_OBJECT:
		{
			fwrite( &node->numChildren, sizeof( uint32_t ), 1, file );
			serialize_node_tree( file, node, fileType );
			break;
		}
	}
}

static void serialize_node_tree( FILE *file, AcmBranch *root, AcmFileType fileType )
{
	AcmBranch *child = root->children.start;
	while ( child != NULL )
	{
		serialize_node( file, child, fileType );
		child = child->next;
	}
}

/**
 * Serialize the given node set.
 */
bool acm_write_file( const char *path, AcmBranch *root, AcmFileType fileType )
{
	FILE *file = fopen( path, "wb" );
	if ( file == NULL )
	{
		set_error_message( ND_ERROR_IO_WRITE, "failed to open path \"%s\"", path );
		return false;
	}

	if ( fileType == ACM_FILE_TYPE_BINARY )
	{
		fprintf( file, ACM_FORMAT_BINARY_HEADER_2 );
		static const unsigned int version = ACM_FORMAT_BINARY_VERSION;
		fwrite( &version, sizeof( uint32_t ), 1, file );
	}
	else
	{
		sDepth = 0;
		fprintf( file, ACM_FORMAT_UTF8_HEADER "\n; this node file has been auto-generated!\n" );
	}

	serialize_node( file, root, fileType );

	fclose( file );

	return true;
}

/******************************************/
/** API Testing **/

void acm_print_tree( AcmBranch *self, int index )
{
	for ( int i = 0; i < index; ++i ) printf( "\t" );
	if ( self->type == ACM_PROPERTY_TYPE_OBJECT || self->type == ACM_PROPERTY_TYPE_ARRAY )
	{
		index++;

		const char *name = ( self->name.buf != NULL ) ? self->name.buf : "";
		if ( self->type == ACM_PROPERTY_TYPE_OBJECT )
		{
			Message( "%s (%s)\n", name, string_for_property_type( self->type ) );
		}
		else
		{
			Message( "%s (%s %s)\n", name, string_for_property_type( self->type ), string_for_property_type( self->childType ) );
		}

		AcmBranch *child = acm_get_first_child( self );
		while ( child != NULL )
		{
			acm_print_tree( child, index );
			child = acm_get_next_child( child );
		}
	}
	else
	{
		AcmBranch *parent = acm_get_parent( self );
		if ( parent != NULL && parent->type == ACM_PROPERTY_TYPE_ARRAY )
		{
			Message( "%s %s\n", string_for_property_type( self->type ), self->data.buf );
		}
		else
		{
			Message( "%s %s %s\n", string_for_property_type( self->type ), self->name.buf, self->data.buf );
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Testing
/////////////////////////////////////////////////////////////////////////////////////

#if defined( ACM_TEST )

int main( PL_UNUSED int argc, PL_UNUSED char **argv )
{
	PlInitialize( argc, argv );

	AcmBranch *branch = acm_load_file( "projects/base/base.prj.n", NULL );
	if ( branch == NULL )
	{
		return EXIT_FAILURE;
	}

	acm_print_tree( branch, 0 );

	return EXIT_SUCCESS;
}

#endif
