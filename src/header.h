#include <iostream>
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <ctype.h>
#include <vector>
#include <list>
#include <algorithm>
#include <climits>
#include <unistd.h>
#include "omp.h"
#include "mpi.h"

using namespace std;

#define MAX_STRING_LENGTH 1024
#define MAX_WORD_LENGTH 20
#define MAX_TEMP_READER_LENGTH 5		// lower seems to be faster, but somehow 1 is higher than 10
#define MAX_TEMP_MAPPER_LENGTH 3000000		// longer seems to be faster -> concern about memory
#define MAX_NUM_BUCKET 6000				// longer seems to be faster -> concern about memory
										// number of English word in the dictionary is 171,476
#define MAX_LOCK_ARRAY_SIZE 10
