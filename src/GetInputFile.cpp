#include "GetInputFile.h"

void GetInputFile(char** argv, vector<char*>& list_file)
{
	// declare variables
	char* file_name = new char[5 * MAX_WORD_LENGTH];
	const char s[5] = "\n\r\0";

	// open the list_file.txt to get the list of files to work on
	FILE* p_list_file;
	p_list_file = fopen(argv[1], "r");

	while (fgets(file_name, 5 * MAX_WORD_LENGTH, p_list_file))
	{
		file_name = strtok(file_name, s);		// strip the file_name of \n and \r

		char* temp = new char[5 * MAX_WORD_LENGTH];
		strcpy(temp, file_name);					// copy file name to the memory location pointed by temp
		list_file.push_back(temp);					// add the file's name to the list for processing
	}

	// free up memory
	delete[] file_name;
}
