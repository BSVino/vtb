#define VTB_MEMORY_LAYOUT_IMPLEMENTATION

#include "../vtb_memory_layout.h"

#include <stdlib.h>
#include <stdio.h>

#define VTB_IMPLEMENTATION

#include "../vtb.h"

struct MLTest
{
	int junk1;
	float junk2;
	int* int_field;
	float* float_field;
};

int main()
{
	int num_ints = 12;
	int num_floats = 6*3;

	VTBML_BEGIN(MLTest, test1)
		VTBML_ENTRY(test1, int_field, num_ints)
		VTBML_ENTRY(test1, float_field, num_floats)
	VTBML_END()

	int memory_required = VTBML_GET_MEMORY_REQUIRED(test1);

	printf("Memory required: %d\n", memory_required);

	MLTest* laidout = VTBML_LAYOUT(MLTest, test1, malloc(memory_required));

	laidout->junk1 = 1;
	laidout->junk2 = 2;
	for (int k = 0; k < num_ints; k++)
		laidout->int_field[k] = k+2;
	for (int k = 0; k < num_floats; k++)
		laidout->float_field[k] = k+num_ints+2;

	VAssert(laidout->junk1 == 1);
	VAssert(laidout->junk2 == 2);
	for (int k = 0; k < num_ints; k++)
		VAssert(laidout->int_field[k] == k+2);
	for (int k = 0; k < num_floats; k++)
		VAssert(laidout->float_field[k] == k+num_ints+2);
}
