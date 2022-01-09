# Map-Reduce-with-MPI-and-OpenMP
Implementation of a parallel word count program with MapReduce pattern using MPI and OpenMP APIs in C++

1. To compile, navigate to /src folder and run "make" command. Output program is main.exe.

2. The input text files are to be stored in /RawText folder, their path names are to be copied to RawText.txt for processing.

3. Command to run the program: mpiexec -n [num_processors] ./main RawText.txt 1 [num_mapper_per_processor]

4. Final word counts are written to /output.txt

5. Benchmarking performance data for a total of 181M words is shown below:

![1111](https://user-images.githubusercontent.com/35698328/56923723-7f93cd00-6a88-11e9-8609-c63afacbc834.JPG)

![2222](https://user-images.githubusercontent.com/35698328/56923735-88849e80-6a88-11e9-9a86-038a2a460939.JPG)
