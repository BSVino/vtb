#define VTB_ALLOC_RING_IMPLEMENTATION

#include "../vtb_alloc_ring.h"

#include <stdio.h>
#include <string.h>

const char* g_test;
int g_line;

static void catch_sigbus(int signal)
{
    printf("Bus error during test '%s' after line %d\n", g_test, g_line);
    exit(1);
}

static void catch_sigfpe(int signal)
{
    printf("Floating point exception during test '%s' after line %d\n", g_test, g_line);
    exit(1);
}

static void catch_sigill(int signal)
{
    printf("Illegal instruction during test '%s' after line %d\n", g_test, g_line);
    exit(1);
}

static void catch_sigsegv(int signal)
{
    printf("Segfault during test '%s' after line %d\n", g_test, g_line);
    exit(1);
}

#define TEST(x) g_line = __LINE__; { if (!(x)) { printf("Test '" #x "' on line %d during '%s' failed.\n", __LINE__, g_test); return 1; } }

int main()
{
	if (signal(SIGBUS, catch_sigbus) == SIG_ERR ||
		signal(SIGFPE, catch_sigfpe) == SIG_ERR ||
		signal(SIGILL, catch_sigill) == SIG_ERR ||
		signal(SIGSEGV, catch_sigsegv) == SIG_ERR)
	{
		fputs("An error occurred while setting a signal handler.\n", stderr);
		return 1;
	}

	const char* test_string1 = "abcd1234aoeu";
	const char* test_string2 = "aoeulcrg1234";

	void* memory;
	int length;
	int p;

	g_test = "Initial test";

	vtb_ring_allocator a;
	char m[1024];
	vtbar_initialize(&a, m, sizeof(m));

	TEST(vtbar_isusermemory(&a) == 1);
	TEST(vtbar_getmemorysize(&a) == sizeof(m));

	TEST(vtbar_isempty(&a));
	TEST(vtbar_getnumallocations(&a) == 0);
	TEST(vtbar_getsizeallocations(&a) == 0);

	vtbar_peektail(&a, &memory, &length);
	TEST(!memory && !length);

	strcpy((char*)vtbar_alloc(&a, 16), test_string1);

	TEST(!vtbar_isempty(&a));
	TEST(vtbar_getnumallocations(&a) == 1);
	TEST(vtbar_getsizeallocations(&a) == 16 + vtbar_getheadersize());

	vtbar_peektail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	vtbar_freetail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	TEST(vtbar_isempty(&a));
	TEST(vtbar_getnumallocations(&a) == 0);
	TEST(vtbar_getsizeallocations(&a) == 0);

	strcpy((char*)vtbar_alloc(&a, 16), test_string1);

	TEST(!vtbar_isempty(&a));
	TEST(vtbar_getnumallocations(&a) == 1);
	TEST(vtbar_getsizeallocations(&a) == 16 + vtbar_getheadersize()*vtbar_getnumallocations(&a));

	vtbar_peektail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	strcpy((char*)vtbar_alloc(&a, 16), test_string2);

	TEST(!vtbar_isempty(&a));
	TEST(vtbar_getnumallocations(&a) == 2);
	TEST(vtbar_getsizeallocations(&a) == 32 + vtbar_getheadersize()*vtbar_getnumallocations(&a));

	vtbar_peektail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	vtbar_freetail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	TEST(!vtbar_isempty(&a));
	TEST(vtbar_getnumallocations(&a) == 1);
	TEST(vtbar_getsizeallocations(&a) == 16 + vtbar_getheadersize()*vtbar_getnumallocations(&a));

	vtbar_peektail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string2) == 0 && length == 16);

	TEST(!vtbar_isempty(&a));
	TEST(vtbar_getnumallocations(&a) == 1);
	TEST(vtbar_getsizeallocations(&a) == 16 + vtbar_getheadersize()*vtbar_getnumallocations(&a));

	vtbar_freetail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string2) == 0 && length == 16);

	vtbar_peektail(&a, &memory, &length);
	TEST(memory == 0 && length == 0);
	vtbar_freetail(&a, &memory, &length);
	TEST(memory == 0 && length == 0);

	TEST(vtbar_isempty(&a));
	TEST(vtbar_getnumallocations(&a) == 0);
	TEST(vtbar_getsizeallocations(&a) == 0 + vtbar_getheadersize()*vtbar_getnumallocations(&a));

	vtbar_destroy(&a);

	g_test = "Alignment";
	vtbar_initialize(&a, m, sizeof(m));
	TEST(vtbar_getnumallocations(&a) == 0);

	p = (size_t)(int*)vtbar_alloc(&a, 1) % sizeof(size_t);
	TEST(p == 0);
	TEST(vtbar_getnumallocations(&a) == 1);

	p = (size_t)(int*)vtbar_alloc(&a, 2) % sizeof(size_t);
	TEST(p == 0);
	TEST(vtbar_getnumallocations(&a) == 2);

	p = (size_t)(int*)vtbar_alloc(&a, 3) % sizeof(size_t);
	TEST(p == 0);
	TEST(vtbar_getnumallocations(&a) == 3);

	vtbar_destroy(&a);

	g_test = "Overallocation";
	vtbar_initialize(&a, m, sizeof(m));
	TEST(vtbar_getnumallocations(&a) == 0);
	TEST(vtbar_getsizeallocations(&a) == 0 + vtbar_getheadersize()*vtbar_getnumallocations(&a));

	TEST(vtbar_alloc(&a, sizeof(m)*2) == 0);
	TEST(vtbar_getnumallocations(&a) == 0);
	TEST(vtbar_getsizeallocations(&a) == 0 + vtbar_getheadersize()*vtbar_getnumallocations(&a));

	TEST(vtbar_alloc(&a, sizeof(m)/2) != 0);
	TEST(vtbar_getnumallocations(&a) == 1);
	TEST(vtbar_getsizeallocations(&a) == sizeof(m)/2 + vtbar_getheadersize()*vtbar_getnumallocations(&a));

	TEST(vtbar_alloc(&a, sizeof(m)/2) == 0);
	TEST(vtbar_getnumallocations(&a) == 1);
	TEST(vtbar_getsizeallocations(&a) == sizeof(m)/2 + vtbar_getheadersize()*vtbar_getnumallocations(&a));
	vtbar_destroy(&a);

	g_test = "Allocating across the break";
	vtbar_initialize(&a, m, sizeof(m));
	TEST(vtbar_getnumallocations(&a) == 0);
	TEST(vtbar_getsizeallocations(&a) == 0 + vtbar_getheadersize()*vtbar_getnumallocations(&a));
	TEST(vtbar_alloc(&a, 496) != 0);
	TEST(vtbar_getnumallocations(&a) == 1);
	TEST(vtbar_getsizeallocations(&a) == 496 + vtbar_getheadersize()*vtbar_getnumallocations(&a));
	TEST(vtbar_alloc(&a, 496) != 0);
	TEST(vtbar_getnumallocations(&a) == 2);
	TEST(vtbar_getsizeallocations(&a) == 992 + vtbar_getheadersize()*vtbar_getnumallocations(&a));
	vtbar_freetail(&a, &memory, &length);
	TEST(vtbar_getnumallocations(&a) == 1);
	TEST(vtbar_getsizeallocations(&a) == 496 + vtbar_getheadersize()*vtbar_getnumallocations(&a));

	TEST(vtbar_alloc(&a, 496) != 0);
	TEST(vtbar_getnumallocations(&a) == 2);
	TEST(vtbar_getsizeallocations(&a) == 992 + vtbar_getheadersize()*vtbar_getnumallocations(&a));
	vtbar_destroy(&a);

	vtbar_initialize(&a, m, sizeof(m));
	TEST(vtbar_getnumallocations(&a) == 0);
	TEST(vtbar_getsizeallocations(&a) == 0 + vtbar_getheadersize()*vtbar_getnumallocations(&a));
	TEST(vtbar_alloc(&a, 304) != 0);
	TEST(vtbar_getnumallocations(&a) == 1);
	TEST(vtbar_getsizeallocations(&a) == 304 + vtbar_getheadersize()*vtbar_getnumallocations(&a));
	TEST(vtbar_alloc(&a, 304) != 0);
	TEST(vtbar_getnumallocations(&a) == 2);
	TEST(vtbar_getsizeallocations(&a) == 608 + vtbar_getheadersize()*vtbar_getnumallocations(&a));
	TEST(vtbar_alloc(&a, 304) != 0);
	TEST(vtbar_getnumallocations(&a) == 3);
	TEST(vtbar_getsizeallocations(&a) == 912 + vtbar_getheadersize()*vtbar_getnumallocations(&a));
	vtbar_freetail(&a, &memory, &length);
	TEST(vtbar_getnumallocations(&a) == 2);
	TEST(vtbar_getsizeallocations(&a) == 608 + vtbar_getheadersize()*vtbar_getnumallocations(&a));

	TEST(vtbar_alloc(&a, 496) == 0);
	TEST(vtbar_getnumallocations(&a) == 2);
	TEST(vtbar_getsizeallocations(&a) == 608 + vtbar_getheadersize()*vtbar_getnumallocations(&a));
	vtbar_destroy(&a);

	vtbar_initialize(&a, m, sizeof(m));
	TEST(vtbar_alloc(&a, 300) != 0);
	TEST(vtbar_alloc(&a, 300) != 0);
	TEST(vtbar_alloc(&a, 300) != 0);
	vtbar_freetail(&a, &memory, &length);

	TEST(vtbar_alloc(&a, 200) != 0);
	vtbar_destroy(&a);

	g_test = "Allocating after the break";
	vtbar_initialize(&a, m, sizeof(m));
	TEST(vtbar_alloc(&a, 496) != 0);
	TEST(vtbar_alloc(&a, 496) != 0);
	vtbar_freetail(&a, &memory, &length);
	TEST(length == 496);

	TEST(vtbar_alloc(&a, 96) != 0);
	TEST(vtbar_alloc(&a, 96) != 0);
	TEST(vtbar_alloc(&a, 96) != 0);
	TEST(vtbar_alloc(&a, 96) != 0);
	vtbar_freetail(&a, &memory, &length);
	TEST(length == 496);
	vtbar_freetail(&a, &memory, &length);
	TEST(length == 96);
	vtbar_freetail(&a, &memory, &length);
	TEST(length == 96);
	vtbar_freetail(&a, &memory, &length);
	TEST(length == 96);
	TEST(vtbar_alloc(&a, 96) != 0);
	TEST(vtbar_alloc(&a, 96) != 0);
	vtbar_freetail(&a, &memory, &length);
	TEST(length == 96);
	vtbar_freetail(&a, &memory, &length);
	TEST(length == 96);
	vtbar_freetail(&a, &memory, &length);
	TEST(length == 96);

	vtbar_destroy(&a);

#ifndef VTBAR_NO_MALLOC
	g_test = "Automatic malloc test";

	vtbar_initializememory(&a, 1024);
	TEST(vtbar_isusermemory(&a) == 0);
	TEST(vtbar_getmemorysize(&a) == 1024);

	TEST(vtbar_isempty(&a));

	vtbar_peektail(&a, &memory, &length);
	TEST(!memory && !length);

	strcpy((char*)vtbar_alloc(&a, 16), test_string1);

	TEST(!vtbar_isempty(&a));

	vtbar_peektail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	strcpy((char*)vtbar_alloc(&a, 16), test_string2);

	TEST(!vtbar_isempty(&a));

	vtbar_peektail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	vtbar_freetail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	TEST(!vtbar_isempty(&a));

	vtbar_peektail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string2) == 0 && length == 16);

	TEST(!vtbar_isempty(&a));

	vtbar_freetail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string2) == 0 && length == 16);

	vtbar_peektail(&a, &memory, &length);
	TEST(memory == 0 && length == 0);
	vtbar_freetail(&a, &memory, &length);
	TEST(memory == 0 && length == 0);

	TEST(vtbar_isempty(&a));

	vtbar_destroy(&a);


	g_test = "Exact number of items test";

	vtbar_initializeitems(&a, 10, 16);
	TEST(vtbar_isusermemory(&a) == 0);
	TEST(vtbar_getmemorysize(&a) == (16+vtbar_getheadersize())*10);

	strcpy((char*)vtbar_alloc(&a, 16), test_string1);
	strcpy((char*)vtbar_alloc(&a, 16), test_string2);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);

	strcpy((char*)vtbar_alloc(&a, 16), test_string1);
	strcpy((char*)vtbar_alloc(&a, 16), test_string2);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);

	strcpy((char*)vtbar_alloc(&a, 16), test_string1);
	strcpy((char*)vtbar_alloc(&a, 16), test_string2);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);

	strcpy((char*)vtbar_alloc(&a, 16), test_string1);
	strcpy((char*)vtbar_alloc(&a, 16), test_string2);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);

	strcpy((char*)vtbar_alloc(&a, 16), test_string1);
	strcpy((char*)vtbar_alloc(&a, 16), test_string2);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);

	strcpy((char*)vtbar_alloc(&a, 16), test_string1);
	strcpy((char*)vtbar_alloc(&a, 16), test_string2);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);

	strcpy((char*)vtbar_alloc(&a, 16), test_string1);
	strcpy((char*)vtbar_alloc(&a, 16), test_string2);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);

	strcpy((char*)vtbar_alloc(&a, 16), test_string1);
	strcpy((char*)vtbar_alloc(&a, 16), test_string2);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);

	strcpy((char*)vtbar_alloc(&a, 16), test_string1);
	strcpy((char*)vtbar_alloc(&a, 16), test_string2);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);

	strcpy((char*)vtbar_alloc(&a, 16), test_string1);
	TEST(vtbar_alloc(&a, 16) == 0);

	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbar_alloc(&a, 16), test_string2);

	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbar_alloc(&a, 16), test_string1);

	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbar_alloc(&a, 16), test_string2);

	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbar_alloc(&a, 16), test_string1);

	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbar_alloc(&a, 16), test_string2);

	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbar_alloc(&a, 16), test_string1);

	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbar_alloc(&a, 16), test_string2);

	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbar_alloc(&a, 16), test_string1);

	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbar_alloc(&a, 16), test_string2);

	vtbar_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);

	vtbar_destroy(&a);
#endif

	return 0;
}


