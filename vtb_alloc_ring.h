/*
vtb_alloc_ring.h - public domain ring allocator

This software is in the public domain. Where that dedication is not
recognized, you are granted a perpetual, irrevocable license to copy
and modify this file as you see fit. No warranty is offered or implied.

This is a memory allocator implementing a ring buffer from a block of memory
that you optionally provide. It has constant time alloc and free, making it a
compelling replacement for a linked list in places where memory locality is
important. It does not require memory copies and always returns contiguous
memory blocks. This implementation is not thread safe.


COMPILING AND LINKING
	You must

	#define VTB_ALLOC_RING_IMPLEMENTATION

	in exactly one C++ file that includes this header, before the include
	like this:

	#define VTB_ALLOC_RING_IMPLEMENTATION
	#include "vtb_alloc_ring.h"

	All other files can be just #include "vtb_alloc_ring.h" without the #define


QUICK START
	vtb_ring_allocator a;
	vtbar_initializememory(&a, 4096); // Allocates 4096 bytes with malloc

	vec3* link1 = (vec3*)vtbar_alloc(&a, sizeof(vec3));
	*link1 = vec3(1, 2, 3);

	int* link2 = (vec3*)vtbar_alloc(&a, sizeof(int));
	*link2 = 42;

	vec3* result1;
	int length;
	vtbar_freetail(&a, (void**)&result1, &length);
	// Now *result1 == vec3(1, 2, 3) and length == sizeof(vec3)

	int* result2;
	vtbar_freetail(&a, (void**)&result2, &length);
	// Now *result2 == 42 and length == sizeof(int)

	// Now the buffer is empty.

	vtbar_destroy(&a);


MEMORY MANAGEMENT
	Note that some of the data in the memory provided will be used for state
	management, so if you allocate sizeof(YourStructure)*k you will be able
	to fit strictly fewer than k YourStructures into the ring buffer. Use
	vtbar_getheadersize() to account for this: If you allocate
	(sizeof(YourStructure) + vtbar_getheadersize())*k then you will have space
	for exactly k items in the ring buffer. vtbar_initializeitems() does this
	for you.

	Note: Because the algorithm always returns contiguous memory blocks, if you
	don't have a constant object size and use vtbar_initializeitems, you may
	have empty space at the end of the buffer. There's not much to be done
	this if we don't copy memory around, which I want to avoid, but memory is
	pretty plentiful these days.

	If you use vtbar_initialize() then no memory will be allocated. If you use

	#define VTBAR_NO_MALLOC

	then you can avoid #include stdlib.h


ASSERT
	Define VTBAR_ASSERT(boolval) to override assert() and not use assert.h
*/

#ifndef VTB__ALLOC_RING_H
#define VTB__ALLOC_RING_H

#ifdef VTBAR_STATIC
#define VTBARDEF static
#else
#ifdef __cplusplus
#define VTBARDEF extern "C"
#else
#define VTBARDEF extern
#endif
#endif

#include <stdint.h> // For uint8_t/int32_t

#define VTB__PRIVATE_MEMBER(type, name) type vtb__##name

// WARNING: Don't directly reference members of this struct. I reserve
// the right to change them from version to version.
// VTB__PRIVATE_MEMBER is here to discourage you from trying to reference
// them. Use the API procedures provided instead.
typedef struct
{
	VTB__PRIVATE_MEMBER(uint8_t*, m_memory);
	VTB__PRIVATE_MEMBER(int32_t, m_memory_size);

	// m_head/tail_index are indexes into m_memory
	VTB__PRIVATE_MEMBER(int32_t, m_head_index); // Head is the most recently alloc'd section
	VTB__PRIVATE_MEMBER(int32_t, m_tail_index); // Tail is the section about to be freed

	VTB__PRIVATE_MEMBER(int32_t, m_num_allocations);
	VTB__PRIVATE_MEMBER(int32_t, m_size_allocations);

	VTB__PRIVATE_MEMBER(uint8_t, m_flags); // Currently only contains the free flag.
} vtb_ring_allocator;

// Use this initializer if you want VRingAllocator to use the memory that
// you provide.
VTBARDEF void vtbar_initialize(vtb_ring_allocator* vtbra, void* memory, int32_t memory_size);

// This initializer will allocate memory for you, for convenience.
// It will be freed when you call Destroy().
VTBARDEF void vtbar_initializememory(vtb_ring_allocator* vtbra, int32_t memory_size);

// This initializer will allocate memory for you, for convenience,
// an amount exactly enough to fit this many items.
VTBARDEF void vtbar_initializeitems(vtb_ring_allocator* vtbra, int32_t items, int32_t sizeof_item);

// Deallocates memory.
VTBARDEF void vtbar_destroy(vtb_ring_allocator* vtbra);

// Request a section of memory.
// If it returns 0, that means there was no space.
// The item will be placed at the "head" of the list. You will always
// receive a contiguous block of memory in return.
VTBARDEF void* vtbar_alloc(vtb_ring_allocator* vtbra, int32_t size);

// Return the item least recently allocated, but does not free it.
VTBARDEF void vtbar_peektail(vtb_ring_allocator* vtbra, void** start, int32_t* length);

// Return and free the item least recently allocated.
VTBARDEF void vtbar_freetail(vtb_ring_allocator* vtbra, void** start, int32_t* length);

// Return true if the list is empty, false otherwise.
VTBARDEF int vtbar_isempty(vtb_ring_allocator* vtbra);

// Returns the total number of allocations. Incremented by alloc, decremented by free.
VTBARDEF int vtbar_getnumallocations(vtb_ring_allocator* vtbra);

// Returns the total size of all allocations. Incremented by alloc, decremented by free.
VTBARDEF int vtbar_getsizeallocations(vtb_ring_allocator* vtbra);

// Returns the total amount of memory available.
VTBARDEF int vtbar_getmemorysize(vtb_ring_allocator* vtbra);

// Returns 1 when the allocator is using memory passed into vtbar_initialize, 0 otherwise.
// When returning 1, the allocator will not free when vtbar_destroy is called. When returning 0, it will.
VTBARDEF int vtbar_isusermemory(vtb_ring_allocator* vtbra);

// Returns the size of the header structure used for state management.
// Exactly one such structure is created for each allocation, packed
// tightly.
VTBARDEF int vtbar_getheadersize();

#endif // VTB__ALLOC_RING_H



#ifdef VTB_ALLOC_RING_IMPLEMENTATION

#ifndef VTBAR_ASSERT
#include <assert.h>
#define VTBAR_ASSERT(x) assert(x)
#endif

#ifdef VTBAR_DEBUG
#define VTBAR__ASSERT VTBAR_ASSERT
#define VTBAR__CHECK VTBAR_ASSERT
#else
#define VTBAR__ASSERT(x)
#define VTBAR__CHECK VTBAR_ASSERT
#endif

#ifndef VTBAR_NO_MALLOC
#include <stdlib.h>
#endif

typedef struct
{
	int32_t m_length; // Allocation size.
	int32_t m_next;   // Index into m_memory. Points to the header of the next block.
} vtb__memory_section_header;

VTBARDEF void vtbar_initialize(vtb_ring_allocator* vtbra, void* memory, int32_t memory_size)
{
	VTBAR__CHECK(memory);
	VTBAR__CHECK(memory_size > sizeof(vtb__memory_section_header) && memory_size < 99999999);
	VTBAR__CHECK(((size_t)(size_t*)memory) % sizeof(size_t) == 0); // Can't handle unaligned memory.

	vtbra->vtb__m_memory = (uint8_t*)memory;
	vtbra->vtb__m_memory_size = memory_size;
	vtbra->vtb__m_head_index = vtbra->vtb__m_tail_index = -1;
	vtbra->vtb__m_flags = 0;
	vtbra->vtb__m_num_allocations = 0;
	vtbra->vtb__m_size_allocations = 0;
}

VTBARDEF void vtbar_initializememory(vtb_ring_allocator* vtbra, int32_t memory_size)
{
#ifndef VTBAR_NO_MALLOC
	VTBAR__CHECK(memory_size > sizeof(vtb__memory_section_header) && memory_size < 99999999);

	vtbar_initialize(vtbra, malloc(memory_size), memory_size);

	vtbra->vtb__m_flags = 1;
#else
	vtbra = vtbra;
	memory_size = memory_size;
	VTBAR__CHECK(false);
#endif
}

VTBARDEF void vtbar_initializeitems(vtb_ring_allocator* vtbra, int32_t items, int32_t sizeof_item)
{
#ifndef VTBAR_NO_MALLOC
	VTBAR__CHECK(items > 0 && items < 99999999);
	VTBAR__CHECK(sizeof_item > 0 && sizeof_item < 99999999);

	int memory_size = (sizeof(vtb__memory_section_header) + sizeof_item) * items;
	vtbar_initialize(vtbra, malloc(memory_size), memory_size);

	vtbra->vtb__m_flags = 1;
#else
	sizeof_item = sizeof_item;
	items = items;
	vtbra = vtbra;
	VTBAR__CHECK(false);
#endif
}

VTBARDEF void vtbar_destroy(vtb_ring_allocator* vtbra)
{
#ifndef VTBAR_NO_MALLOC
	if (vtbra->vtb__m_flags)
	{
		VTBAR__CHECK(vtbra->vtb__m_memory); // Double free
		free(vtbra->vtb__m_memory);
	}
#endif

	vtbra->vtb__m_memory = 0;
}

VTBARDEF void* vtbar_alloc(vtb_ring_allocator* vtbra, int32_t size)
{
	VTBAR__CHECK(size > 0);
	VTBAR__CHECK(vtbra->vtb__m_memory); // Call initialize first

	if (size%(int32_t)sizeof(size_t) != 0)
		size += (int32_t)sizeof(size_t) - size%(int32_t)sizeof(size_t);

	if (vtbra->vtb__m_head_index < 0)
	{
		// This is the first block allocated.
		if ((int32_t)sizeof(vtb__memory_section_header) + size > vtbra->vtb__m_memory_size)
		{
			VTBAR__ASSERT(false);
			return 0;
		}

		vtbra->vtb__m_num_allocations++;
		vtbra->vtb__m_size_allocations += size + (int32_t)sizeof(vtb__memory_section_header);

		vtbra->vtb__m_head_index = vtbra->vtb__m_tail_index = 0;

		vtb__memory_section_header* header = (vtb__memory_section_header*)vtbra->vtb__m_memory;
		header->m_length = size;
		header->m_next = -1;

		return (void*)(header+1);
	}

	int limit = vtbra->vtb__m_memory_size;
	if (vtbra->vtb__m_head_index < vtbra->vtb__m_tail_index)
		limit = vtbra->vtb__m_tail_index;

	vtb__memory_section_header* header = (vtb__memory_section_header*)&vtbra->vtb__m_memory[vtbra->vtb__m_head_index];
	if (vtbra->vtb__m_head_index + header->m_length + 2*(int)sizeof(vtb__memory_section_header) + size <= limit)
	{
		vtbra->vtb__m_num_allocations++;
		vtbra->vtb__m_size_allocations += size + (int)sizeof(vtb__memory_section_header);

		vtbra->vtb__m_head_index += (int)sizeof(vtb__memory_section_header) + header->m_length;
		vtb__memory_section_header* new_header = (vtb__memory_section_header*)&vtbra->vtb__m_memory[vtbra->vtb__m_head_index];

		new_header->m_length = size;
		new_header->m_next = -1;

		header->m_next = vtbra->vtb__m_head_index;

		return (void*)(new_header+1);
	}
	// Not enough room at the end. Is there enough room at the beginning?
	else if (vtbra->vtb__m_head_index >= vtbra->vtb__m_tail_index && (int)sizeof(vtb__memory_section_header) + size <= vtbra->vtb__m_tail_index)
	{
		vtbra->vtb__m_num_allocations++;
		vtbra->vtb__m_size_allocations += size + (int)sizeof(vtb__memory_section_header);

		vtbra->vtb__m_head_index = 0;

		vtb__memory_section_header* new_header = (vtb__memory_section_header*)&vtbra->vtb__m_memory[vtbra->vtb__m_head_index];

		new_header->m_length = size;
		new_header->m_next = -1;

		header->m_next = vtbra->vtb__m_head_index;

		return (void*)(new_header+1);
	}

	return 0;
}

VTBARDEF void vtbar_peektail(vtb_ring_allocator* vtbra, void** start, int32_t* length)
{
	VTBAR__CHECK(vtbra->vtb__m_memory); // Call initialize first

	if (vtbar_isempty(vtbra))
	{
		*start = 0;
		*length = 0;
		return;
	}

	VTBAR__ASSERT(vtbra->vtb__m_tail_index >= 0);

	uint8_t* memory = &vtbra->vtb__m_memory[vtbra->vtb__m_tail_index];
	vtb__memory_section_header* header = (vtb__memory_section_header*)memory;
	*length = header->m_length;
	*start = (void*)(header+1);
}

VTBARDEF void vtbar_freetail(vtb_ring_allocator* vtbra, void** start, int32_t* length)
{
	VTBAR__CHECK(vtbra->vtb__m_memory); // Call initialize first

	if (vtbar_isempty(vtbra))
	{
		VTBAR__ASSERT(false);
		*start = 0;
		*length = 0;
		return;
	}

	VTBAR__ASSERT(vtbra->vtb__m_tail_index >= 0);

	vtb__memory_section_header* header = (vtb__memory_section_header*)&vtbra->vtb__m_memory[vtbra->vtb__m_tail_index];

	if (length)
		*length = header->m_length;

	if (start)
		*start = (void*)(header+1);

	if (header->m_next >= 0)
		vtbra->vtb__m_tail_index = header->m_next;
	else
		vtbra->vtb__m_head_index = vtbra->vtb__m_tail_index = -1;

	vtbra->vtb__m_num_allocations--;
	vtbra->vtb__m_size_allocations -= header->m_length + (int)sizeof(vtb__memory_section_header);
}

VTBARDEF int vtbar_isempty(vtb_ring_allocator* vtbra)
{
	VTBAR__CHECK(vtbra->vtb__m_memory); // Call initialize first

	return vtbra->vtb__m_head_index == -1;
}

VTBARDEF int vtbar_getnumallocations(vtb_ring_allocator* vtbra)
{
	VTBAR__CHECK(vtbra->vtb__m_memory); // Call initialize first

	return vtbra->vtb__m_num_allocations;
}

VTBARDEF int vtbar_getsizeallocations(vtb_ring_allocator* vtbra)
{
	VTBAR__CHECK(vtbra->vtb__m_memory); // Call initialize first

	return vtbra->vtb__m_size_allocations;
}

VTBARDEF int vtbar_getmemorysize(vtb_ring_allocator* vtbra)
{
	VTBAR__CHECK(vtbra->vtb__m_memory); // Call initialize first

	return vtbra->vtb__m_memory_size;
}

VTBARDEF int vtbar_isusermemory(vtb_ring_allocator* vtbra)
{
	VTBAR__CHECK(vtbra->vtb__m_memory); // Call initialize first

	return !vtbra->vtb__m_flags;
}

VTBARDEF int vtbar_getheadersize()
{
	return sizeof(vtb__memory_section_header);
}

#endif