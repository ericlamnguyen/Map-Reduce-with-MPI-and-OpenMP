#include "Writer.h"

void Writer(vector<vector<Item>>& hash_table)
{
	// open my_file for writing
	ofstream my_file;
	my_file.open("output.txt");
	
	int length_curr_bucket;

	for (int i = 0; i < MAX_NUM_BUCKET; i++)
	{
		length_curr_bucket = hash_table[i].size();

		if (length_curr_bucket > 0)
		{
			for (int j = 0; j < length_curr_bucket; j++)
			{
				my_file << hash_table[i][j].word << ": " << hash_table[i][j].count << endl;
			}
		}
	}
	
	// close my_file
	my_file.close();
}
