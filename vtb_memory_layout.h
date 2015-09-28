/*
vtb_memory_layout.h - public domain memory layout tool

This software is in the public domain. Where that dedication is not
recognized, you are granted a perpetual, irrevocable license to copy
and modify this file as you see fit. No warranty is offered or implied.

This is a memory layout tool that uses user-provided data about a
variable-size memory structure to assign pointers safely. This
implementation is not thread safe.


COMPILING AND LINKING
	You must

	#define VTB_MEMORY_LAYOUT_IMPLEMENTATION

	in exactly one C++ file that includes this header, before the include
	like this:

	#define VTB_MEMORY_LAYOUT_IMPLEMENTATION
	#include "vtb_memory_layout.h"

	All other files can be just #include "vtb_memory_layout.h" without the #define


QUICK START


ASSERT
	Define VTBML_ASSERT(boolval) to override assert() and not use assert.h
*/

#ifndef VTB__MEMORY_LAYOUT_H
#define VTB__MEMORY_LAYOUT_H

#ifdef VTBML_STATIC
#define VTBMLDEF static
#else
#ifdef __cplusplus
#define VTBMLDEF extern "C"
#else
#define VTBMLDEF extern
#endif
#endif

struct VTBML_Entry
{
	int m_size;
	int m_offset;
};

#define VTBML_BEGIN(StructName, Identifier) \
	struct StructName Identifier##_instance; \
	VTBML_Entry Identifier##_entries[] = { \
		{ sizeof(StructName), 0 }, \

#define VTBML_ENTRY(Identifier, FieldName, Count) \
		{ (int)(sizeof(Identifier##_instance.FieldName)*(Count)), (int)((char*)((void*)&Identifier##_instance.FieldName)-(char*)(&Identifier##_instance)) },

#define VTBML_END() \
	}; \

#define VTBML_GET_MEMORY_REQUIRED(Identifier) \
	vtbml_get_memory_required(VArraySize(Identifier##_entries), Identifier##_entries);

#define VTBML_LAYOUT(StructName, Identifier, Memory) \
	(struct StructName*)(vtbml_layout_memory(VArraySize(Identifier##_entries), Identifier##_entries, Memory));

VTBMLDEF int vtbml_get_memory_required(int num_entries, struct VTBML_Entry* entries);
VTBMLDEF void* vtbml_layout_memory(int num_entries, struct VTBML_Entry* entries, void* memory);

#ifndef VArraySize
#define VArraySize(x) (sizeof(x)/sizeof(x[0]))
#endif

#endif // VTB__MEMORY_LAYOUT_H



#ifdef VTB_MEMORY_LAYOUT_IMPLEMENTATION

#ifndef VTBML_ASSERT
#include <assert.h>
#define VTBML_ASSERT(x) assert(x)
#endif

#ifdef VTBML_DEBUG
#define VTBML__ASSERT VTBML_ASSERT
#define VTBML__CHECK VTBML_ASSERT
#else
#define VTBML__ASSERT(x)
#define VTBML__CHECK VTBML_ASSERT
#endif

VTBMLDEF int vtbml_get_memory_required(int num_entries, struct VTBML_Entry* entries)
{
	int r = 0;
	for (int k = 0; k < num_entries; k++)
		r += entries[k].m_size;

	return r;
}

VTBMLDEF void* vtbml_layout_memory(int num_entries, struct VTBML_Entry* entries, void* memory)
{
	// First entry is the struct itself
	int current = entries[0].m_size;
	char* memory_char = (char*)memory;

	for (int k = 1; k < num_entries; k++)
	{
		char** pointer = (char**)(memory_char + entries[k].m_offset);
		*pointer = memory_char + current;
		current += entries[k].m_size;
	}

	return memory;
}

#endif

