#define VTB_HASH_IMPLEMENTATION

#include "../vtb_hash.h"

#define VTB_IMPLEMENTATION
#include "../vtb.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

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

#define TEST(x) g_line = __LINE__; { if (!(x)) { printf("Test '" #x "' on line %d during '%s' failed.\n", __LINE__, g_test); test = 1; } }

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

	int test = 0;

	vtb_hash a = vtbh_new();
	vtb_hash b = vtbh_new();
	TEST(a.hash == b.hash);

	unsigned char bytes[] =
	{
		0x47,
		0xac,
		0x52,
		0xf1,
	};

	vtbh_bytes(&a, bytes, sizeof(bytes));

	for (int k = 0; k < sizeof(bytes); k++)
		vtbh_byte(&b, bytes[k]);

	TEST(a.hash == b.hash);

	a = vtbh_new();

	uint32_t h1 = a.hash;

	vtbh_byte(&a, 0);

	uint32_t h2 = a.hash;

	// Ensure a large change even for a zero byte hash.
	TEST(abs((int32_t)h1 - (int32_t)h2) > 0x10000000);

	double sum = 0;

	srand(0);

	a = vtbh_new();

	// Any particular sample will have
	// this probability of being picked.
	double chance = 0.01;
	double n = log(1-chance)/log(1-pow(2, -32));

	uint64_t num_tests = n+1;

	double m, s;

	uint64_t num_buckets = 512*1024;
	uint64_t num_possible_samples = uint64_t((uint32_t)~0)+1;
	int samples_per_bucket = num_possible_samples/num_buckets;

	unsigned int buckets[num_buckets];
	memset(buckets, 0, sizeof(buckets));

	int sizeof_one_bucket = sizeof(unsigned int) * samples_per_bucket;
	unsigned int* one_bucket = (unsigned int*)malloc(sizeof_one_bucket);
	memset(one_bucket, 0, sizeof_one_bucket);

	uint64_t even = 0;

	uint64_t bytes_hashed = 0;

	for (int k = 0; k < num_tests; k++)
	{
		b = a;

		srand(k);

		unsigned char bytes[rand()%10+10];

		for (int j = 0; j < sizeof(bytes); j++)
			bytes[j] = (unsigned char)rand();

		vtbh_bytes(&b, bytes, sizeof(bytes));

		bytes_hashed += sizeof(bytes);

		double sample = double(b.hash);

		sum += sample;

		if (k == 0)
		{
			m = sample;
			s = 0;
		}
		else
		{
			double m_prev = m;
			m = m + (sample - m)/(k+1);
			s = s + (sample - m_prev)*(sample - m);
		}

		int bucket = b.hash/(num_possible_samples/num_buckets);
		buckets[bucket]++;

		//if (b.hash < VArraySize(one_bucket))
		//	one_bucket[b.hash]++;

		even += !(b.hash%2);
	}

	double max_value = double((uint32_t)~0);
	double expected_mean = max_value/2;
	double expected_variance = (1.0f/12)*(max_value*max_value);

	double mean_confidence_interval_997 = 3*sqrt(expected_variance/num_tests);

	double mean = sum/num_tests;

	double mean_difference = fabs(mean - expected_mean);

	TEST(mean_difference < mean_confidence_interval_997);

	double sample_variance = s/(num_tests-1);

	double variance_distribution_mean = expected_variance;
	double variance_distribution_variance = 2*expected_variance*expected_variance/(num_tests-1);

	double variance_difference = sample_variance - variance_distribution_mean;

	double variance_z_score = variance_difference/sqrt(variance_distribution_variance);

	TEST(fabs(variance_z_score) < 3);

	double expected_even = num_tests/2;
	double expected_odd = expected_even;
	double odd = num_tests - even;
	double even_difference = even - expected_even;
	double odd_difference = odd - expected_odd;
	double even_odd_chi_squared = even_difference*even_difference/expected_even + odd_difference*odd_difference/expected_odd;

	TEST(even_odd_chi_squared < 10);

	double chi_sum = 0;
	double expected_samples_per_bucket = num_tests/num_buckets;
	for (int k = 0; k < num_buckets; k++)
	{
		double diff = buckets[k] - expected_samples_per_bucket;
		chi_sum += diff*diff;
	}

	chi_sum /= expected_samples_per_bucket;

	TEST(num_buckets == 512*1024); // The following value depends on this number of buckets.
	double buckets_chi_squared_value = 527457.0340;

	TEST(chi_sum < buckets_chi_squared_value);

	/*
	for (int k = 0; k < num_buckets; k++)
		printf("buckets[%d] = %d\n", k, (int)buckets[k]-(int)expected_samples_per_bucket);

	for (int k = 0; k < VArraySize(one_bucket); k++)
		printf("one_bucket[%d] = %u\n", k, one_bucket[k]);
	*/

	return test;
}


