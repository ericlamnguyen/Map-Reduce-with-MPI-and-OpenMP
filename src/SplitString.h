#ifndef _SPLIT_STRING_H
#define _SPLIT_STRING_H

#include "header.h"

void SplitString(char* file_name, list<string>& list_words, omp_lock_t* lock_list_words, int pid);

#endif
