#include "isqrt.h"

uint32_t isqrt(uint32_t n)
{
	uint32_t root = 0, bit, trial;
	bit = (n >= 0x10000) ? (uint32_t)1<<30 : (uint32_t)1<<14;
	do
	{
		trial = root+bit;
		if (n >= trial)
		{
			n -= trial;
			root = trial+bit;
		}
		root >>= 1;
		bit >>= 2;
	} while (bit);
	return root;
}
