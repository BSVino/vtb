#define VTB_MEMORY_LAYOUT_IMPLEMENTATION

#include "../vtb_memory_layout.h"

#include <stdlib.h>
#include <stdio.h>

#define VTB_IMPLEMENTATION

#include "../vtb.h"

struct MLTestSub
{
	int junk1;
	double* double_field;
};

struct MLTest
{
	int        junk1;
	float      junk2;
	int*       int_field;
	float*     float_field;
	MLTestSub* sub;
	char*      char_field;
};

int main()
{
	int num_ints = 12;
	int num_floats = 6*3;
	int num_chars = 4;
	int num_subs = 6;
	int num_doubles = 200;

	VTBML_USE_SUBSTRUCT(test1, MLTestSub)
	VTBML_BEGIN(test1, MLTest)
		VTBML_ENTRY(test1, int_field, num_ints)
		VTBML_ENTRY(test1, float_field, num_floats)
		VTBML_ENTRY(test1, sub, num_subs)
			VTBML_SUB_ENTRY(test1, MLTestSub, double_field, num_doubles, 1)
		VTBML_ENTRY(test1, char_field, num_chars)
	VTBML_END()

	int memory_required = VTBML_GET_MEMORY_REQUIRED(test1);

	printf("Memory required: %d\n", memory_required);

	MLTest* laidout = VTBML_LAYOUT(MLTest, test1, malloc(memory_required));

	int n = 0;
	laidout->junk1 = n++;
	laidout->junk2 = n++;
	for (int k = 0; k < num_ints; k++)
		laidout->int_field[k] = n++;
	for (int k = 0; k < num_floats; k++)
		laidout->float_field[k] = n++;
	for (int k = 0; k < num_subs; k++)
	{
		laidout->sub[k].junk1 = n++;
		for (int k2 = 0; k2 < num_doubles; k2++)
			laidout->sub[k].double_field[k2] = n++;
	}
	for (int k = 0; k < num_chars; k++)
		laidout->char_field[k] = n++;

	n = 0;
	VAssert(laidout->junk1 == n++);
	VAssert(laidout->junk2 == n++);
	for (int k = 0; k < num_ints; k++)
		VAssert(laidout->int_field[k] == n++);
	for (int k = 0; k < num_floats; k++)
		VAssert(laidout->float_field[k] == n++);
	for (int k = 0; k < num_subs; k++)
	{
		VAssert(laidout->sub[k].junk1 == n++);
		for (int k2 = 0; k2 < num_doubles; k2++)
			VAssert(laidout->sub[k].double_field[k2] == n++);
	}
	for (int k = 0; k < num_chars; k++)
		VAssert(laidout->char_field[k] == n++);
}
