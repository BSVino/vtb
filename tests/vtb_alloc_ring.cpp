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

	g_test = "Initial test";

	vtb_ring_allocator a;
	char m[1024];
	vtbra_initialize(&a, m, sizeof(m));

	TEST(vtbra_isusermemory(&a) == 1);
	TEST(vtbra_getmemorysize(&a) == sizeof(m));

	TEST(vtbra_isempty(&a));
	TEST(vtbra_getnumallocations(&a) == 0);
	TEST(vtbra_getsizeallocations(&a) == 0);

	void* memory;
	int length;
	vtbra_peektail(&a, &memory, &length);
	TEST(!memory && !length);

	strcpy((char*)vtbra_alloc(&a, 16), test_string1);

	TEST(!vtbra_isempty(&a));
	TEST(vtbra_getnumallocations(&a) == 1);
	TEST(vtbra_getsizeallocations(&a) == 16 + vtbra_getheadersize());

	vtbra_peektail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	vtbra_freetail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	TEST(vtbra_isempty(&a));
	TEST(vtbra_getnumallocations(&a) == 0);
	TEST(vtbra_getsizeallocations(&a) == 0);

	strcpy((char*)vtbra_alloc(&a, 16), test_string1);

	TEST(!vtbra_isempty(&a));
	TEST(vtbra_getnumallocations(&a) == 1);
	TEST(vtbra_getsizeallocations(&a) == 16 + vtbra_getheadersize()*vtbra_getnumallocations(&a));

	vtbra_peektail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	strcpy((char*)vtbra_alloc(&a, 16), test_string2);

	TEST(!vtbra_isempty(&a));
	TEST(vtbra_getnumallocations(&a) == 2);
	TEST(vtbra_getsizeallocations(&a) == 32 + vtbra_getheadersize()*vtbra_getnumallocations(&a));

	vtbra_peektail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	vtbra_freetail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	TEST(!vtbra_isempty(&a));
	TEST(vtbra_getnumallocations(&a) == 1);
	TEST(vtbra_getsizeallocations(&a) == 16 + vtbra_getheadersize()*vtbra_getnumallocations(&a));

	vtbra_peektail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string2) == 0 && length == 16);

	TEST(!vtbra_isempty(&a));
	TEST(vtbra_getnumallocations(&a) == 1);
	TEST(vtbra_getsizeallocations(&a) == 16 + vtbra_getheadersize()*vtbra_getnumallocations(&a));

	vtbra_freetail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string2) == 0 && length == 16);

	vtbra_peektail(&a, &memory, &length);
	TEST(memory == 0 && length == 0);
	vtbra_freetail(&a, &memory, &length);
	TEST(memory == 0 && length == 0);

	TEST(vtbra_isempty(&a));
	TEST(vtbra_getnumallocations(&a) == 0);
	TEST(vtbra_getsizeallocations(&a) == 0 + vtbra_getheadersize()*vtbra_getnumallocations(&a));

	vtbra_destroy(&a);

	g_test = "Overallocation";
	vtbra_initialize(&a, m, sizeof(m));
	TEST(vtbra_getnumallocations(&a) == 0);
	TEST(vtbra_getsizeallocations(&a) == 0 + vtbra_getheadersize()*vtbra_getnumallocations(&a));

	TEST(vtbra_alloc(&a, sizeof(m)*2) == 0);
	TEST(vtbra_getnumallocations(&a) == 0);
	TEST(vtbra_getsizeallocations(&a) == 0 + vtbra_getheadersize()*vtbra_getnumallocations(&a));

	TEST(vtbra_alloc(&a, sizeof(m)/2) != 0);
	TEST(vtbra_getnumallocations(&a) == 1);
	TEST(vtbra_getsizeallocations(&a) == sizeof(m)/2 + vtbra_getheadersize()*vtbra_getnumallocations(&a));

	TEST(vtbra_alloc(&a, sizeof(m)/2) == 0);
	TEST(vtbra_getnumallocations(&a) == 1);
	TEST(vtbra_getsizeallocations(&a) == sizeof(m)/2 + vtbra_getheadersize()*vtbra_getnumallocations(&a));
	vtbra_destroy(&a);

	g_test = "Allocating across the break";
	vtbra_initialize(&a, m, sizeof(m));
	TEST(vtbra_getnumallocations(&a) == 0);
	TEST(vtbra_getsizeallocations(&a) == 0 + vtbra_getheadersize()*vtbra_getnumallocations(&a));
	TEST(vtbra_alloc(&a, 500) != 0);
	TEST(vtbra_getnumallocations(&a) == 1);
	TEST(vtbra_getsizeallocations(&a) == 500 + vtbra_getheadersize()*vtbra_getnumallocations(&a));
	TEST(vtbra_alloc(&a, 500) != 0);
	TEST(vtbra_getnumallocations(&a) == 2);
	TEST(vtbra_getsizeallocations(&a) == 1000 + vtbra_getheadersize()*vtbra_getnumallocations(&a));
	vtbra_freetail(&a, &memory, &length);
	TEST(vtbra_getnumallocations(&a) == 1);
	TEST(vtbra_getsizeallocations(&a) == 500 + vtbra_getheadersize()*vtbra_getnumallocations(&a));

	TEST(vtbra_alloc(&a, 500) != 0);
	TEST(vtbra_getnumallocations(&a) == 2);
	TEST(vtbra_getsizeallocations(&a) == 1000 + vtbra_getheadersize()*vtbra_getnumallocations(&a));
	vtbra_destroy(&a);

	vtbra_initialize(&a, m, sizeof(m));
	TEST(vtbra_getnumallocations(&a) == 0);
	TEST(vtbra_getsizeallocations(&a) == 0 + vtbra_getheadersize()*vtbra_getnumallocations(&a));
	TEST(vtbra_alloc(&a, 300) != 0);
	TEST(vtbra_getnumallocations(&a) == 1);
	TEST(vtbra_getsizeallocations(&a) == 300 + vtbra_getheadersize()*vtbra_getnumallocations(&a));
	TEST(vtbra_alloc(&a, 300) != 0);
	TEST(vtbra_getnumallocations(&a) == 2);
	TEST(vtbra_getsizeallocations(&a) == 600 + vtbra_getheadersize()*vtbra_getnumallocations(&a));
	TEST(vtbra_alloc(&a, 300) != 0);
	TEST(vtbra_getnumallocations(&a) == 3);
	TEST(vtbra_getsizeallocations(&a) == 900 + vtbra_getheadersize()*vtbra_getnumallocations(&a));
	vtbra_freetail(&a, &memory, &length);
	TEST(vtbra_getnumallocations(&a) == 2);
	TEST(vtbra_getsizeallocations(&a) == 600 + vtbra_getheadersize()*vtbra_getnumallocations(&a));

	TEST(vtbra_alloc(&a, 500) == 0);
	TEST(vtbra_getnumallocations(&a) == 2);
	TEST(vtbra_getsizeallocations(&a) == 600 + vtbra_getheadersize()*vtbra_getnumallocations(&a));
	vtbra_destroy(&a);

	vtbra_initialize(&a, m, sizeof(m));
	TEST(vtbra_alloc(&a, 300) != 0);
	TEST(vtbra_alloc(&a, 300) != 0);
	TEST(vtbra_alloc(&a, 300) != 0);
	vtbra_freetail(&a, &memory, &length);

	TEST(vtbra_alloc(&a, 200) != 0);
	vtbra_destroy(&a);

	g_test = "Allocating after the break";
	vtbra_initialize(&a, m, sizeof(m));
	TEST(vtbra_alloc(&a, 500) != 0);
	TEST(vtbra_alloc(&a, 500) != 0);
	vtbra_freetail(&a, &memory, &length);
	TEST(length == 500);

	TEST(vtbra_alloc(&a, 100) != 0);
	TEST(vtbra_alloc(&a, 100) != 0);
	TEST(vtbra_alloc(&a, 100) != 0);
	TEST(vtbra_alloc(&a, 100) != 0);
	vtbra_freetail(&a, &memory, &length);
	TEST(length == 500);
	vtbra_freetail(&a, &memory, &length);
	TEST(length == 100);
	vtbra_freetail(&a, &memory, &length);
	TEST(length == 100);
	vtbra_freetail(&a, &memory, &length);
	TEST(length == 100);
	TEST(vtbra_alloc(&a, 100) != 0);
	TEST(vtbra_alloc(&a, 100) != 0);
	vtbra_freetail(&a, &memory, &length);
	TEST(length == 100);
	vtbra_freetail(&a, &memory, &length);
	TEST(length == 100);
	vtbra_freetail(&a, &memory, &length);
	TEST(length == 100);

	vtbra_destroy(&a);

#ifndef VTBAR_NO_MALLOC
	g_test = "Automatic malloc test";

	vtbra_initializememory(&a, 1024);
	TEST(vtbra_isusermemory(&a) == 0);
	TEST(vtbra_getmemorysize(&a) == 1024);

	TEST(vtbra_isempty(&a));

	vtbra_peektail(&a, &memory, &length);
	TEST(!memory && !length);

	strcpy((char*)vtbra_alloc(&a, 16), test_string1);

	TEST(!vtbra_isempty(&a));

	vtbra_peektail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	strcpy((char*)vtbra_alloc(&a, 16), test_string2);

	TEST(!vtbra_isempty(&a));

	vtbra_peektail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	vtbra_freetail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string1) == 0 && length == 16);

	TEST(!vtbra_isempty(&a));

	vtbra_peektail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string2) == 0 && length == 16);

	TEST(!vtbra_isempty(&a));

	vtbra_freetail(&a, &memory, &length);
	TEST(strcmp((char*)memory, test_string2) == 0 && length == 16);

	vtbra_peektail(&a, &memory, &length);
	TEST(memory == 0 && length == 0);
	vtbra_freetail(&a, &memory, &length);
	TEST(memory == 0 && length == 0);

	TEST(vtbra_isempty(&a));

	vtbra_destroy(&a);


	g_test = "Exact number of items test";

	vtbra_initializeitems(&a, 10, 16);
	TEST(vtbra_isusermemory(&a) == 0);
	TEST(vtbra_getmemorysize(&a) == (16+vtbra_getheadersize())*10);

	strcpy((char*)vtbra_alloc(&a, 16), test_string1);
	strcpy((char*)vtbra_alloc(&a, 16), test_string2);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);

	strcpy((char*)vtbra_alloc(&a, 16), test_string1);
	strcpy((char*)vtbra_alloc(&a, 16), test_string2);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);

	strcpy((char*)vtbra_alloc(&a, 16), test_string1);
	strcpy((char*)vtbra_alloc(&a, 16), test_string2);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);

	strcpy((char*)vtbra_alloc(&a, 16), test_string1);
	strcpy((char*)vtbra_alloc(&a, 16), test_string2);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);

	strcpy((char*)vtbra_alloc(&a, 16), test_string1);
	strcpy((char*)vtbra_alloc(&a, 16), test_string2);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);

	strcpy((char*)vtbra_alloc(&a, 16), test_string1);
	strcpy((char*)vtbra_alloc(&a, 16), test_string2);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);

	strcpy((char*)vtbra_alloc(&a, 16), test_string1);
	strcpy((char*)vtbra_alloc(&a, 16), test_string2);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);

	strcpy((char*)vtbra_alloc(&a, 16), test_string1);
	strcpy((char*)vtbra_alloc(&a, 16), test_string2);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);

	strcpy((char*)vtbra_alloc(&a, 16), test_string1);
	strcpy((char*)vtbra_alloc(&a, 16), test_string2);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);

	strcpy((char*)vtbra_alloc(&a, 16), test_string1);
	TEST(vtbra_alloc(&a, 16) == 0);

	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbra_alloc(&a, 16), test_string2);

	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbra_alloc(&a, 16), test_string1);

	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbra_alloc(&a, 16), test_string2);

	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbra_alloc(&a, 16), test_string1);

	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbra_alloc(&a, 16), test_string2);

	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbra_alloc(&a, 16), test_string1);

	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbra_alloc(&a, 16), test_string2);

	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbra_alloc(&a, 16), test_string1);

	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);
	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string1) == 0);
	strcpy((char*)vtbra_alloc(&a, 16), test_string2);

	vtbra_freetail(&a, &memory, &length); TEST(length == 16 && strcmp((char*)memory, test_string2) == 0);

	vtbra_destroy(&a);
#endif

	return 0;
}


