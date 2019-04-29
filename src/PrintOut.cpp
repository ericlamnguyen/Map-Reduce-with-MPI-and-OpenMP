#include "PrintOut.h"

void PrintOut(vector<vector<Item>>& hash_table, int* total_count)
{
	int length_curr_bucket;

	for (int i = 0; i < MAX_NUM_BUCKET; i++)
	{
		length_curr_bucket = hash_table[i].size();

		if (length_curr_bucket > 0)
		{
			for (int j = 0; j < length_curr_bucket; j++)
			{
				//cout << hash_table[i][j].word << ": " << hash_table[i][j].count << endl;
				*total_count += hash_table[i][j].count;
			}
		}
	}
}
