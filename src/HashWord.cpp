#include "HashWord.h"

unsigned long HashWord(string word)
{
	unsigned long hash_val = 5381;
	int i;

	for (char c : word)
	{
		i = c;
		hash_val = ((hash_val << 5) + hash_val) + i;		/* hash * 33 + c */
	}

	return (hash_val % MAX_NUM_BUCKET);
}
