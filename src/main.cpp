#include "header.h"

#include "GetInputFile.h"		// GetInputFile()
#include "SplitString.h"		// SplitString()
#include "HashWord.h"			// HashWord()
#include "Item.h"				// Item()
#include "Mapper.h"				// Mapper()
#include "PrintOut.h"			// PrintOut()
#include "Writer.h"				// Writer()

int main(int argc, char** argv) {

	omp_set_dynamic(0);		// explicitly disable dynamic teams
	omp_set_nested(1);		// enable nested parallelism

	// declare variables
	int pid, numP;		// MPI node id and number of nodes

	double time_program = omp_get_wtime();			// to time the program

	char* request_termination_message = "LIST_COMPLETED";		// when reader thread receives this file name from node 0, it knows that there is no more file to be requested from node 0

	int num_readers = atoi(argv[2]);	// number of reader threads
	int num_mappers = atoi(argv[3]);	// number of mapper threads

	/************************/
	/**** initialize MPI ****/
	/************************/

	int provided;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
	MPI_Comm_rank(MPI_COMM_WORLD, &pid);
	MPI_Comm_size(MPI_COMM_WORLD, &numP);

	// start the timer
	// time_program = MPI_Wtime();

	/*********************************/
	/**** Reader + Mapper Threads ****/
	/*********************************/

	// node 0 will first collect all input text file names, afater that it will concurrently distribute them to other nodes while also have a team of reader and mapper threads to pull work from the queue
	if (pid == 0)
	{
		// a list to hold all the words from the text files assigned to this node, reader threads will push_back and mapper threads will pop_front
		list<string> list_words;				

		vector<vector<Item>> hash_table;		// Item<word, count>
		hash_table.resize(MAX_NUM_BUCKET);

		// use to signal to mapper threads that reader threads have finished
		// when reader threads finish, *readers_finished = 1
		int* readers_finished = new int();		
		*readers_finished = 0;		

		// create a list of file names to be distributed
		vector<char*> list_file;

		/******************/
		// generate locks
		/******************/
		// create lock for list_words
		omp_lock_t lock_list_words;				
		omp_init_lock(&lock_list_words);

		// create lock for list_file
		omp_lock_t lock_list_file;				
		omp_init_lock(&lock_list_file);
		
		// create an array of 10 locks to enable efficient access to shared hash_table among the mapper threads
		// lock_array[i] will control the access to buckets according to the rule: i = bucket's index % MAX_LOCK_ARRAY_SIZE;
		omp_lock_t lock_array[MAX_LOCK_ARRAY_SIZE];
		for (int i = 0; i < MAX_LOCK_ARRAY_SIZE; i++)
		{
			omp_init_lock(&lock_array[i]);
		}

		/**************************/
		// Finish generating locks
		/**************************/

		// pre-populate list_file with special file name "LIST_COMPLETED" so that when a reader thread receives this file name it knows that there is no more file to be requested from node 0 and exit
		for (int i = 0; i < (numP - 1) * num_readers; i++)
		{
			list_file.push_back(request_termination_message);
		}

		// add the file names to list_file
		GetInputFile(argv, list_file);					// argv[1] is a text file that contains the input text files to be processed, all the text file names will be added to list_file

		/*****************************************************************************/
		// create a team of 2 threads
		// thread 0 will distribute the works from the queue to other nodes
		// thread 1 will create a team of reader threads and a team of mapper threads
		/*****************************************************************************/
		#pragma omp parallel sections
		{
			/******************************************************************************************/
			// 1 thread distribute the work in the queue to other nodes via MPI_Send and MPI_Recv
			// this is only needed if there are more than 1 node running the program
			/******************************************************************************************/
			#pragma omp section
			{
				if (numP > 1)
				{
					int mpi_ping = 0;

					MPI_Status request_stats;			// for MPI_recv

					// listen to request for file from other nodes as long as there is file left in list_file
					while (list_file.size() > 0)
					{
						MPI_Recv(&mpi_ping, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &request_stats);		// listen to MPI ping for work from other nodes

						omp_set_lock(&lock_list_file);		// set lock		
						MPI_Send(list_file.back(), 5 * MAX_WORD_LENGTH, MPI_CHAR, request_stats.MPI_SOURCE, request_stats.MPI_SOURCE, MPI_COMM_WORLD);			// send the file to respective node
						list_file.pop_back();		// remove the file that was just sent
						omp_unset_lock(&lock_list_file);	// unset lock
					}

					// print out to console
					cout << "*** ALL WORKS HAVE BEEN DISTRIBUTED ***" << endl;
				}
			}
			
			/************************************************************************/
			// 1 thread creates a team of reader threads and a team of mapper threads
			/************************************************************************/
			#pragma omp section
			{
				/************************************************/
				// create a team of 2 threads
				// 1 thread will create a team of reader threads
				// 1 thread will create a team of mapper threads
				/************************************************/
				#pragma omp parallel sections
				{
					/********************************************/
					// 1 thread creates a team of reader threads
					/********************************************/
					#pragma omp section
					{
						// start the timer
						double time_reader = omp_get_wtime();
						double time_reader_useful = 0;
						double time_reader_temp;

						#pragma omp parallel num_threads(num_readers)
						{
							char* file_name;		// holder for file name to work on

							while (1)
							{
								omp_set_lock(&lock_list_file);		// set lock on list_file

								if (list_file.size() > 0)			// if the list is not empty
								{
									file_name = list_file.back();			// look at the last file in the list

									if (strcmp(file_name, request_termination_message) != 0)		// if the last file in the list is not the request_termination_message 
									{
										list_file.pop_back();					// remove the file that was just pulled
										omp_unset_lock(&lock_list_file);		// unset the lock
									}
									// if the last file in the list is the request_termination_message, do nothing to the list, unset the lock and exit the while loop 
									else
									{
										omp_unset_lock(&lock_list_file);
										break;
									}
								}
								// if there is no more file in the list, unset the lock and exit the while loop
								else
								{
									omp_unset_lock(&lock_list_file);
									break;
								}

								// reader thread to read the text file and add the words to local list_words
								if (strcmp(file_name, request_termination_message) != 0)
								{
									time_reader_temp = omp_get_wtime();
									SplitString(file_name, list_words, &lock_list_words, pid);
									time_reader_useful += (omp_get_wtime() - time_reader_temp);
								}
							}
						}
						
						// signal to mapper threads that reader threads have finished
						#pragma omp critical	// critical is used to ensure flush is done
						{
							*readers_finished = 1;
						}

						// *** DEBUG ***
						cout << "PID: " << pid << " - TOTAL COUNT BY READER: " << list_words.size() << endl;
						cout << "PID: " << pid << " - READERS FINISHED - TOTAL RUNTIME: " << omp_get_wtime() - time_reader << endl;
						cout << "PID: " << pid << " - READERS FINISHED - USEFUL RUNTIME: " << time_reader_useful << endl;

					}
					
					/********************************************/
					// 1 thread creates a team of mapper threads
					/********************************************/
					#pragma omp section
					{
						// *** DEBUG ***
						while (*readers_finished == 0)
						{
							usleep(100);
						}
						
						

						// create a team of mapper threads
						#pragma omp parallel num_threads(num_mappers)
						{
							// create local list<string> temp_list_mapper
							// each thread will pull a chunk of words from list_words to its local temp_list_mapper to work on
							list<string> temp_list_mapper;
							list<string>::iterator it;		
							int curr_list_words_size;

							// start the timer
							double time_mapper = omp_get_wtime();
							double time_mapper_useful = 0;
							double time_mapper_temp;

							int max_temp_list_size = list_words.size() / (1 * num_mappers);

							// pull words from list_words and map to hash_table
							while (1)
							{
								omp_set_lock(&lock_list_words);
								curr_list_words_size = list_words.size();
								// when there are words in the queue, pull the words to temp_list_mapper and unset the lock
								if (curr_list_words_size > 0)
								{
									if (curr_list_words_size <= max_temp_list_size)		// if the available words in list_words is less than MAX_TEMP_LENGTH, get all the words to temp_list_mapper
									{
										time_mapper_temp = omp_get_wtime();
										temp_list_mapper.splice(temp_list_mapper.begin(), list_words, list_words.begin(), list_words.end());
										omp_unset_lock(&lock_list_words);
										Mapper(hash_table, temp_list_mapper, lock_array);
										time_mapper_useful += (omp_get_wtime() - time_mapper_temp);
									}
									else
									{
										time_mapper_temp = omp_get_wtime();
										it = list_words.begin();
										advance(it, max_temp_list_size);
										temp_list_mapper.splice(temp_list_mapper.begin(), list_words, list_words.begin(), it);
										omp_unset_lock(&lock_list_words);
										Mapper(hash_table, temp_list_mapper, lock_array);
										time_mapper_useful += (omp_get_wtime() - time_mapper_temp);
									}
								}
								// when there is no more words and all reader threads have finished, unset the lock and exit the loop
								else
								{
									omp_unset_lock(&lock_list_words);
									break;
								}
							}

							//*** DEBUG ***
							cout << "PID: " << pid << " - THREAD " << omp_get_thread_num() << " - MAPPER FINISHED - TOTAL RUNTIME: " << omp_get_wtime() - time_mapper << endl;
							cout << "PID: " << pid << " - THREAD " << omp_get_thread_num() << " - MAPPER FINISHED - TOTAL USEFUL TIME: " << time_mapper_useful << endl;
						}

						// *** DEBUG ***
						//int total_count = 0;
						//PrintOut(hash_table, &total_count);
						//cout << "PID: " << pid << " - TOTAL COUNT BY MAPPER: " << total_count << endl;

					}
				}
			}
		}

		// destroy locks
		omp_destroy_lock(&lock_list_file);
		omp_destroy_lock(&lock_list_words);

		// free up memory
		delete readers_finished;

		/*******************************************/
		/*** Reader and Mapper threads completed ***/
		/*******************************************/

		/*** DEBUG ***/
		//int total_count11 = 0;
		//PrintOut(hash_table, &total_count11);
		//cout << "MASTER - COUNT BEFORE REDUCING: " << total_count11 << endl;

		/********************************************************************************/
		/*** pid 0 will receive hash_table from other nodes to perform final reducing ***/
		/*** this is only needed if more than 1 node is running the program           ***/
		/********************************************************************************/
		if (numP > 1)
		{
			#pragma omp parallel for num_threads(numP - 1)
			for (int i = 1; i < numP; i++)
			{
				// start the timer
				double time_reducer = omp_get_wtime();
				double time_reducer_useful = 0;
				double time_reducer_temp;
				
				int word_array_size = 0;
				int count_array_size = 0;

				// receive word_array's size from node i
				MPI_Recv(&word_array_size, 1, MPI_INT, i, (i * 99), MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				// receive count_array's size from node i
				MPI_Recv(&count_array_size, 1, MPI_INT, i, (i * 99), MPI_COMM_WORLD, MPI_STATUS_IGNORE);

				// create memory buffer to receive hash_table content from node i
				char word_array[word_array_size];
				int count_array[count_array_size];
				//int index_array[count_array_size];

				// receive word_array size from node i
				MPI_Recv(word_array, word_array_size, MPI_CHAR, i, (i * 99), MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				// receive count_array from node i
				MPI_Recv(count_array, count_array_size, MPI_INT, i, (i * 99), MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				// receive index_array from node i
				//MPI_Recv(index_array, count_array_size, MPI_INT, i, (i * 99), MPI_COMM_WORLD, MPI_STATUS_IGNORE);


				/*** DEBUG ***/
				//int total_count123 = 0;
				//for (int d = 0; d < count_array_size; d++)
				//{
				//	total_count123 += count_array[d];
				//}
				//cout << "MASTER - RECEIVED FROM PID: " << i << " - TOTAL COUNT: " << total_count123 << endl;
				//cout << "PID - 0 - DATA RECEIVED FROM NODE: " << i << endl;

				time_reducer_temp = omp_get_wtime();

				/***********************************************************/
				// extract words from word_array and add to a vector<string>
				/***********************************************************/
				// initialize a vector<string>
				vector<string> list_words_reducer;

				const char s[3] = "/";			// list of characters to be used as delimiters to extract words from the text
				char* word = new char[MAX_WORD_LENGTH];		// current word

				word = strtok(word_array, s);	// get the first word

				// walk through other tokens
				while (word != NULL)
				{
					// add the word to list_words_reducer
					list_words_reducer.push_back(string(word));

					// get the next word
					word = strtok(NULL, s);
				}

				// free memory
				delete[] word;

				/*****************************/
				// Word extraction completes
				/*****************************/

				/*** DEBUG ***/
				if (count_array_size != list_words_reducer.size())
				{
					//cout << "COUNT_ARRAY_SIZE AND LIST_WORDS_REDUCER DO NOT MATCH !!!" << endl;
					//cout << "LIST_WORDS_REDUCER SIZE: " << list_words_reducer.size() << endl;
					//cout << "COUNT_ARRAY SIZE: " << count_array_size << endl;

				}
				else
				{
					//cout << "COUNT_ARRAY_SIZE AND LIST_WORDS_REDUCER - OK" << endl;
					//cout << "COUNT_ARRAY SIZE: " << count_array_size << endl;
				}

				/*************************************/
				/* Current threads perform reduction */
				/*************************************/
				int index;
				string cur_word;

				for (int k = 0; k < count_array_size; k++)
				{
					cur_word = list_words_reducer[k];

					// obtain the hash value for the word
					index = HashWord(cur_word);
					//index = index_array[k];

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
								hash_table[index][j].count += count_array[k];
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

				/**************************************/
				/* Current threads finished reduction */
				/**************************************/
				
				time_reducer_useful += (omp_get_wtime() - time_reducer_temp);

				//*** DEBUG ***
				cout << "PID - 0 - FINISHED REDUCING DATA SENT BY NODE: " << i << " - TOTAL RUNTIME: " << omp_get_wtime() - time_reducer << endl;
				cout << "PID - 0 - FINISHED REDUCING DATA SENT BY NODE: " << i << " - USEFUL RUNTIME: " << time_reducer_useful << endl;
			}

			#pragma omp barrier

			// destroy lock array
			for (int i = 0; i < MAX_LOCK_ARRAY_SIZE; i++)
			{
				omp_destroy_lock(&lock_array[i]);
			}

			/*** DEBUG ***/
			//int total_count1 = 0;
			//PrintOut(hash_table, &total_count1);
			//cout << "FINAL: " << pid << " - TOTAL COUNT: " << total_count1 << endl;
		}

		/********************/
		// Write out to file
		/********************/
		Writer(hash_table);
	}



	/************************************************/
	// other nodes will request work from node 0
	// each node will have 2 threads
	// 1 thread creates a team of reader threads
	// 1 thread creates a team of mapper threads
	/************************************************/
	if (pid != 0)
	{
		// a list to hold all the words from the text files assigned to this node, reader threads will push_back and mapper threads will pop_front
		list<string> list_words;				
		
		vector<vector<Item>> hash_table;		// Item<word, count>
		hash_table.resize(MAX_NUM_BUCKET);

		// use to signal to mapper threads that reader threads have finished
		// when reader threads finish, *readers_finished = 1
		int* readers_finished = new int();		
		*readers_finished = 0;		

		/*****************/
		// generate locks
		/*****************/
		// create lock for list_words
		omp_lock_t lock_list_words;		
		omp_init_lock(&lock_list_words);

		// create an array of 10 locks to enable efficient access to shared hash_table among the mapper threads
		// lock_array[i] will control the access to buckets according to the rule: i = bucket's index % MAX_LOCK_ARRAY_SIZE;
		omp_lock_t lock_array[MAX_LOCK_ARRAY_SIZE];
		for (int i = 0; i < MAX_LOCK_ARRAY_SIZE; i++)
		{
			omp_init_lock(&lock_array[i]);
		}

		/****************************/
		// Finished generating locks
		/****************************/
		
		/***********************************************/
		// create a team of 2 threads
		// 1 thread will spawn a team of reader threads
		// 1 thread will spawn a team of mapper threads	
		/***********************************************/
		#pragma omp parallel sections
		{
			/********************************************/
			// 1 thread creates a team of reader threads
			/********************************************/
			#pragma omp section
			{
				// start the timer
				double time_reader = omp_get_wtime();
				double time_reader_useful = 0;
				double time_reader_temp;

				#pragma omp parallel num_threads(num_readers)
				{
					int mpi_ping = pid;
					
					char* file_name = new char[5 * MAX_WORD_LENGTH];		// pointer to the file name to be received from node 0

					while (strcmp(file_name, request_termination_message) != 0)		// as long as request_termination_message is not received from node 0, keep requesting for file to work on from node 0
					{
						MPI_Send(&mpi_ping, 1, MPI_INT, 0, pid, MPI_COMM_WORLD);			// send the request for a file to node 0
						MPI_Recv(file_name, 5 * MAX_WORD_LENGTH, MPI_CHAR, 0, pid, MPI_COMM_WORLD, MPI_STATUS_IGNORE);		// receive the file name from node 0

						// reader thread to read the text file and add the words to local list_words
						if (strcmp(file_name, request_termination_message) != 0)
						{
							time_reader_temp = omp_get_wtime();
							SplitString(file_name, list_words, &lock_list_words, pid);
							time_reader_useful += (omp_get_wtime() - time_reader_temp);
						}
					}

					// free up memory
					delete[] file_name;
				}

				// signal to mapper threads that reader threads have finished
				#pragma omp critical	// critical is used to ensure flush is done
				{
					*readers_finished = 1;
				}

				// *** DEBUG ***
				cout << "PID: " << pid << " - TOTAL COUNT BY READER: " << list_words.size() << endl;			
				cout << "PID: " << pid << " - READER FINISHED - TOTAL RUNTIME: " << omp_get_wtime() - time_reader << endl;
				cout << "PID: " << pid << " - READER FINISHED - USEFUL RUNTIME: " << time_reader_useful << endl;
			}
			
			/**************************************************/
			// 1 thread will create a team of mapper threads
			/**************************************************/
			#pragma omp section
			{
				//*** DEBUG ***
				while (*readers_finished == 0)
				{
					usleep(100);
				}

				// create a team of mapper threads
				#pragma omp parallel num_threads(num_mappers)
				{
					// create local list<string> temp_list_mapper
					// each thread will pull a chunk of words from list_words to its local temp_list_mapper to work on
					list<string> temp_list_mapper;
					list<string>::iterator it;
					int curr_list_words_size;

					// start the timer
					double time_mapper = omp_get_wtime();
					double time_mapper_useful = 0;
					double time_mapper_temp;

					int max_temp_list_size = list_words.size() / (1 * num_mappers);

					// pull words from list_words and map to hash_table
					while (1)
					{
						omp_set_lock(&lock_list_words);
						curr_list_words_size = list_words.size();

						// when there are words in the queue, pull the words to temp_list_mapper and unset the lock
						if (curr_list_words_size > 0)
						{
							if (curr_list_words_size <= max_temp_list_size)		// if the available words in list_words is less than MAX_TEMP_LENGTH, get all the words to temp_list_mapper
							{
								time_mapper_temp = omp_get_wtime();
								temp_list_mapper.splice(temp_list_mapper.begin(), list_words, list_words.begin(), list_words.end());
								omp_unset_lock(&lock_list_words);
								Mapper(hash_table, temp_list_mapper, lock_array);
								time_mapper_useful += (omp_get_wtime() - time_mapper_temp);
							}
							else
							{
								time_mapper_temp = omp_get_wtime();
								it = list_words.begin();
								advance(it, max_temp_list_size);
								// get MAX_TEMP_LENGTH words to temp_list_mapper
								temp_list_mapper.splice(temp_list_mapper.begin(), list_words, list_words.begin(), it);
								omp_unset_lock(&lock_list_words);
								Mapper(hash_table, temp_list_mapper, lock_array);
								time_mapper_useful += (omp_get_wtime() - time_mapper_temp);
							}
						}
						// when there is no more words and all reader threads have finished, unset the lock and exit the loop
						else
						{
							omp_unset_lock(&lock_list_words);
							break;
						}
					}

					//*** DEBUG ***
					cout << "PID: " << pid << " - THREAD " << omp_get_thread_num() << " - MAPPER FINISHED - TOTAL RUNTIME: " << omp_get_wtime() - time_mapper << endl;
					cout << "PID: " << pid << " - THREAD " << omp_get_thread_num() << " - MAPPER FINISHED - TOTAL USEFUL TIME: " << time_mapper_useful << endl;
				}

				// *** DEBUG ***
				//int total_count = 0;
				//PrintOut(hash_table, &total_count);
				//cout << "PID: " << pid << " - TOTAL COUNT BY MAPPER: " << total_count << endl;
				
			}
		}

		/*****************/
		// destroy locks
		/*****************/
		omp_destroy_lock(&lock_list_words);

		// destroy lock array
		for (int i = 0; i < MAX_LOCK_ARRAY_SIZE; i++)
		{
			omp_destroy_lock(&lock_array[i]);
		}

		// free up memory
		delete readers_finished;

		/***************************/
		// Finished destroying locks
		/***************************/

		/***************************************/
		// Reader and Mapper threads completed
		/***************************************/

		/*********************************/
		// Prepare data to send to pid 0
		/*********************************/
		double time_data_send = omp_get_wtime();

		int length_curr_bucket;
		string send_string = "";
		list<int> send_count_list;
		//list<int> send_index_list;

		for (int i = 0; i < MAX_NUM_BUCKET; i++)
		{
			length_curr_bucket = hash_table[i].size();

			if (length_curr_bucket > 0)
			{
				for (int j = 0; j < length_curr_bucket; j++)
				{
					send_string = send_string + hash_table[i][j].word + "/";
					send_count_list.push_back(hash_table[i][j].count);
					//send_index_list.push_back(i);
				}
			}
		}

		// convert send_string into char[]
		int word_array_size = send_string.length() + 1;		// + 1 to account for \0 added by c_str()
		char word_array[word_array_size];
		strcpy(word_array, send_string.c_str());

		// convert send_count_list into int[]
		int count_array_size = send_count_list.size();
		int count_array[count_array_size];
		copy(send_count_list.begin(), send_count_list.end(), count_array);

		// convert send_index_list into int[]
		//int index_array[count_array_size];
		//copy(send_index_list.begin(), send_index_list.end(), index_array);

		/*** DEBUG ***/
		//int total_count = 0;
		//for (int m = 0; m < count_array_size; m++)
		//{
		//	total_count += count_array[m];
		//}
		//cout << "PID - " << pid << " - TOTAL COUNT BEFORE SENDING: " << total_count << endl;

		/*******************************************/
		// Finished preparing data to send to pid 0
		/*******************************************/

		/*************************/
		// Sending data to pid 0
		/*************************/
		// send word_array_size to pid 0
		MPI_Send(&word_array_size, 1, MPI_INT, 0, (pid * 99), MPI_COMM_WORLD);
		// send count_array_size to pid 0
		MPI_Send(&count_array_size, 1, MPI_INT, 0, (pid * 99), MPI_COMM_WORLD);

		// send word_array to pid 0
		MPI_Send(word_array, word_array_size, MPI_CHAR, 0, (pid * 99), MPI_COMM_WORLD);
		// send count_array to pid 0
		MPI_Send(count_array, count_array_size, MPI_INT, 0, (pid * 99), MPI_COMM_WORLD);
		// send index_array to pid 0
		//MPI_Send(index_array, count_array_size, MPI_INT, 0, (pid * 99), MPI_COMM_WORLD);

		/*********************************/
		// Complete sending data to pid 0
		/*********************************/

		cout << "PID: " << pid << " - FINISHED SENDING DATA TO NODE 0 - TOTAL RUNTIME: " << omp_get_wtime() - time_data_send << endl;
	}

	// terminate MPI
	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

	time_program = omp_get_wtime() - time_program;
	if (pid == 0)
	{
		cout << "Runtime: " << time_program << "s" << endl;
	}
}