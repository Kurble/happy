#include "halton.h"
#include <cmath>

float halton(int sequence, int i)
{
	float result = 0;
	
	float f = 1;
	while (i > 0)
	{
		f = f / sequence;
		result = result + f * (i % sequence);

		i = (int) floorf((float)i / (float)sequence);
	}

	return result;
}