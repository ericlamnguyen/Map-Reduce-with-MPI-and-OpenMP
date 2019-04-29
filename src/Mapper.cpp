#include "Mapper.h"

void Mapper(vector<vector<Item>>& hash_table, list<string>& temp_list_mapper, omp_lock_t* lock_array)
{
	int index;
	string cur_word;

	for (list<string>::iterator it = temp_list_mapper.begin(); it != temp_list_mapper.end(); ++it)
	{
		cur_word = *it;

		// obtain the hash value for the word
		index = HashWord(cur_word);

		// set lock
		omp_set_lock(&lock_array[index % MAX_LOCK_ARRAY_SIZE]);

		// collision resolution
		if (hash_table[index].size() == 0)
		{
			hash_table[index].push_back(Item(cur_word));
		}
		else
		{
			bool is_found = 0;
			for (int j = 0; j < hash_table[index].size(); j++)
			{
				if (cur_word.compare(hash_table[index][j].word) == 0)
				{
					hash_table[index][j].count++;
					is_found = 1;
					break;
				}
			}
			if (!is_found)
			{
				hash_table[index].push_back(Item(cur_word));
			}
		}
		
		// unset lock
		omp_unset_lock(&lock_array[index % MAX_LOCK_ARRAY_SIZE]);
	}

	// empty the temp_list_mapper
	temp_list_mapper.clear();
}
