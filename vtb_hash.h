/*
vtb_hash.h - public domain hash function

This software is dual-licensed to the public domain and under the
following license: you are granted a perpetual, irrevocable license
to copy, modify, publish, and distribute this file as you see fit.

This is a simple and fast 32-bit hash function. It should not be
used for anything that requires error correcting or security. It
works great for hash tables and error detecting. It is reasonably
uniform over [0, 2^32-1], deterministic on its inputs, and small
changes of the input will produce drastic changes of the output.
Results are dependent on endianness. On my 2.2 Ghz processor I
can hash 4,525,199,690 bytes in 14.1 seconds, or about one hash
per every 7 cycles.

My two primary uses for vtb_hash are as a simple and fast hashing
function and as a check to see if data has changed.

Note: I don't guarantee that some string of bytes will always have
the same hash over all future versions of this library. I may at
some point opt to improve the properties of the output or running
time by changing the hashing function. If that's not cool with you,
lock yourself to one version of this library, or use another hash.


COMPILING AND LINKING
	You must

	#define VTB_HASH_IMPLEMENTATION

	in exactly one C++ file that includes this header, before the include
	like this:

	#define VTB_HASH_IMPLEMENTATION
	#include "vtb_hash.h"

	All other files can be just #include "vtb_hash.h" without the #define


QUICK START
	vtb_hash h = vtbh_new();

	vtbh_string(&h, "string", 6);
	vtbh_int(&h, 42);
	vtbh_floats(&h, float_array, num_floats);

	// Call as many of those as you like. When you're done,
	// the hash will be:

	printf("%x\n", h.hash);


ASSERT
	Define VTBH_ASSERT(boolval) to override assert() and not use assert.h
*/

#ifndef VTB__HASH_H
#define VTB__HASH_H

#ifdef VTBH_STATIC
#define VTBHDEF static
#else
#ifdef __cplusplus
#define VTBHDEF extern "C"
#else
#define VTBHDEF extern
#endif
#endif

#include <stdint.h> // For uint8_t/int32_t

typedef struct
{
	uint32_t hash;
	uint32_t salt;
} vtb_hash;

// This just returns an initialized vtb_hash, and always the same one.
VTBHDEF vtb_hash vtbh_new();

VTBHDEF void vtbh_bytes(vtb_hash* h, const unsigned char* bytes, size_t num_bytes);
VTBHDEF void vtbh_byte(vtb_hash* h, unsigned char byte);

VTBHDEF void vtbh_ints(vtb_hash* h, unsigned int* ints, size_t num_ints);
VTBHDEF void vtbh_int(vtb_hash* h, unsigned int i);

VTBHDEF void vtbh_floats(vtb_hash* h, const float* floats, size_t num_floats);
VTBHDEF void vtbh_float(vtb_hash* h, float f);

VTBHDEF void vtbh_string(vtb_hash* h, const char* s, size_t length);



#endif // VTB__HASH_H



#ifdef VTB_HASH_IMPLEMENTATION

#ifndef VTBH_ASSERT
#include <assert.h>
#define VTBH_ASSERT(x) assert(x)
#endif

#ifdef VTBH_DEBUG
#define VTBH__ASSERT VTBH_ASSERT
#define VTBH__CHECK VTBH_ASSERT
#else
#define VTBH__ASSERT(x)
#define VTBH__CHECK VTBH_ASSERT
#endif

VTBHDEF vtb_hash vtbh_new()
{
	vtb_hash h;

	h.hash = 0x39531FCD;
	h.salt = 0x7A8F05C5;

	return h;
}

VTBHDEF void vtbh_bytes(vtb_hash* h, const unsigned char* bytes, size_t num_bytes)
{
	uint32_t hash = h->hash;
	uint32_t salt = h->salt;

	for (unsigned int k = 0; k < (unsigned int)num_bytes; k++)
	{
		// Cycle all bits by 1. This way, even if the byte
		// we are hashing is 0, we still get a large change.
		uint32_t top_byte = hash >> 31;
		hash = ((hash << 1) | top_byte) + 1;

		top_byte = salt >> 31;
		salt = ((salt << 1) | top_byte) + 1;

		uint32_t filled = bytes[k]|(bytes[k]<<8)|(bytes[k]<<16)|(bytes[k]<<24);

		// The extract part of MT, but with different numbers
		filled ^= filled >> 10;
		filled ^= (filled << 6) & 0xCE962B40;
		filled ^= (filled << 16) & 0x77E30000;
		filled ^= filled >> 19;

		filled ^= salt;

		hash ^= filled;
	}

	h->hash = hash;
	h->salt = salt;
}

VTBHDEF inline void vtbh_byte(vtb_hash* h, unsigned char byte)
{
	vtbh_bytes(h, &byte, 1);
}

VTBHDEF void vtbh_ints(vtb_hash* h, unsigned int* ints, size_t num_ints)
{
	vtbh_bytes(h, (unsigned char*)ints, num_ints * sizeof(unsigned int));
}

VTBHDEF void vtbh_int(vtb_hash* h, unsigned int i)
{
	vtbh_bytes(h, (unsigned char*)&i, sizeof(unsigned int));
}

VTBHDEF void vtbh_floats(vtb_hash* h, const float* floats, size_t num_floats)
{
	vtbh_bytes(h, (unsigned char*)floats, num_floats * sizeof(float));
}

VTBHDEF void vtbh_float(vtb_hash* h, float f)
{
	vtbh_bytes(h, (unsigned char*)&f, sizeof(float));
}

VTBHDEF void vtbh_string(vtb_hash* h, const char* s, size_t length)
{
	vtbh_bytes(h, (unsigned char*)s, length);
}


#endif