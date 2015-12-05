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
	int m_size;   // The amount of memory 
	int m_offset;
	int m_level;
};

#define VTBML_USE_SUBSTRUCT(Identifier, SubStructName) \
	struct SubStructName Identifier##_##SubStructName##_instance; \

#define VTBML_BEGIN(Identifier, StructName) \
	struct StructName Identifier##_instance; \
	VTBML_Entry Identifier##_entries[] = { \
		{ sizeof(StructName), 0 }, \

#define VTBML_ENTRY(Identifier, FieldName, Count) \
		{ (int)(sizeof(Identifier##_instance.FieldName)*(Count)), (int)((char*)((void*)&Identifier##_instance.FieldName)-(char*)(&Identifier##_instance)), 0 },

#define VTBML_SUB_ENTRY(Identifier, SubStructName, FieldName, Count, SubLevel) \
		{ (int)(sizeof(Identifier##_##SubStructName##_instance.FieldName)*(Count)), (int)((char*)((void*)&Identifier##_##SubStructName##_instance.FieldName)-(char*)(&Identifier##_##SubStructName##_instance)), SubLevel },

#define VTBML_END() \
	}; \

#define VTBML_GET_MEMORY_REQUIRED(Identifier) \
	vtbml_get_memory_required(VArraySize(Identifier##_entries), Identifier##_entries);

#define VTBML_LAYOUT(StructName, Identifier, Memory) \
	(struct StructName*)(vtbml_layout_memory(VArraySize(Identifier##_entries), Identifier##_entries, Memory));

VTBMLDEF int vtbml_get_memory_required(int num_entries, struct VTBML_Entry* entries);
VTBMLDEF void* vtbml_layout_memory(int num_entries, struct VTBML_Entry* entries, void* memory);

// This is a debug 
VTBMLDEF void vtbml_check_layout_table(int num_entries, struct VTBML_Entry* entries);

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

void vtbml_check_layout_table_level(int num_entries, struct VTBML_Entry* entries, int level, int start)
{
	// Shitty n^2 algorithm just for debug checking
	for (int k = start; entries[k].m_level == level && k < num_entries; k++)
	{
		for (int i = k+1; i < num_entries; i++)
			// If you hit this it means you have a duplicate field.
			// That is bad and shouldn't happen. It's probably a
			// copy/paste error.
			VTBML__CHECK(entries[k].m_offset != entries[i].m_offset);
	}
}

VTBMLDEF void vtbml_check_layout_table(int num_entries, struct VTBML_Entry* entries)
{
	// Shitty n^2 algorithm just for debug checking
	for (int k = 1; k < num_entries; k++)
	{
		if (entries[k].m_level)
			vtbml_check_layout_table_level(num_entries, entries, entries[k].m_level, k);
		else
		{
			for (int i = k+1; i < num_entries; i++)
			{
				if (entries[i].m_level == 0)
					// If you hit this it means you have a duplicate field.
					// That is bad and shouldn't happen. It's probably a
					// copy/paste error.
					VTBML__CHECK(entries[k].m_offset != entries[i].m_offset);
			}
		}
	}
}

VTBMLDEF void* vtbml_layout_memory(int num_entries, struct VTBML_Entry* entries, void* memory)
{
#ifdef _DEBUG
	vtbml_check_layout_table(num_entries, entries);
#endif

	// First entry is the struct itself
	int current = entries[0].m_size;
	char* memory_char = (char*)memory;

	for (int k = 1; k < num_entries; k++)
	{
		int level = 0;

		char* struct_pointer = memory_char;

		while (level < entries[k].m_level)
		{
			int i;
			// Look for the next higher level
			for (i = k-1; i >= 0; i++)
			{
				if (entries[i].m_level == level)
					break;
			}

			VTBML__ASSERT(i >= 0);

			struct_pointer = (char*)(struct_pointer + entries[i].m_offset);

			level++;
		}

		char** pointer = (char**)(struct_pointer + entries[k].m_offset);
		*pointer = memory_char + current;
		current += entries[k].m_size;
	}

	return memory;
}

#endif

