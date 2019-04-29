#ifndef _MAPPER_H
#define _MAPPER_H

#include "header.h"
#include "Item.h"
#include "HashWord.h"

void Mapper(vector<vector<Item>>& hash_table, list<string>& temp_list_mapper, omp_lock_t* lock_array);

#endif