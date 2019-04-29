#include "SplitString.h"


void SplitString(char* file_name, list<string>& list_words, omp_lock_t* lock_list_words, int pid)
{
	// declare variables
	const char s[50] = " 0123456789/\\!@#$%^&*()-_=+:;'<>,.?~`[]{}\"\n\r\0";		// list of characters to be used as delimiters to extract words from the text
	char* curr_string = new char[MAX_STRING_LENGTH];	// current line
	char* word = new char[MAX_WORD_LENGTH];				// current word
	
	// a temporary list to hold the words from the text file
	// once the list is long enough it will be appended to list_words
	// this is to limit the number of lock setting on list_words	
	//list<string> temp_list_reader;				
	
	// read the input text file
	FILE* p_file;
	p_file = fopen(file_name, "r");

	//// check if file open was successful
	//if (p_file)
	//{
	//	cout << file_name << " - Processed successfully by pid - " << pid << endl;
	//}
	//else
	//{
	//	cout << file_name << " - UNSUCCESSFUL" << endl;
	//}

	// go through each line in the text
	while (fgets(curr_string, MAX_STRING_LENGTH, p_file))
	{
		// turn all the letters to lowercase
		for (int cc = 0; cc <= strlen(curr_string); cc++)
		{
			curr_string[cc] = tolower(curr_string[cc]);
		}

		// get the first word
		word = strtok(curr_string, s);

		// walk through other tokens
		while (word != NULL)
		{
			// add word to list_words
			list_words.push_back(string(word));

			// get the next word
			word = strtok(NULL, s);
		}
	}

	// free memory
	delete[] curr_string;
	delete[] word;
}
