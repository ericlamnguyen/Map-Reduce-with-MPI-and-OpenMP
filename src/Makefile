CC = mpicxx
CFLAGS  = -std=c++11 -fopenmp

default: main

main:  main.o HashWord.o Item.o Mapper.o PrintOut.o SplitString.o GetInputFile.o Writer.o
	$(CC) $(CFLAGS) -o main main.o HashWord.o Item.o Mapper.o PrintOut.o SplitString.o GetInputFile.o Writer.o

main.o: HashWord.cpp HashWord.h Item.cpp Item.h Mapper.cpp Mapper.h PrintOut.cpp PrintOut.h SplitString.cpp SplitString.h header.h
	$(CC) $(CFLAGS) -c main.cpp

HashWord.o: HashWord.cpp HashWord.h header.h
	$(CC) $(CFLAGS) -c HashWord.cpp

Item.o: Item.cpp Item.h header.h
	$(CC) $(CFLAGS) -c Item.cpp

Mapper.o: Mapper.cpp Mapper.h header.h Item.cpp Item.h HashWord.cpp HashWord.h
	$(CC) $(CFLAGS) -c Mapper.cpp

PrintOut.o: PrintOut.cpp PrintOut.h header.h Item.cpp Item.h
	$(CC) $(CFLAGS) -c PrintOut.cpp

SplitString.o: SplitString.cpp SplitString.h header.h
	$(CC) $(CFLAGS) -c SplitString.cpp

GetInputFile.o: GetInputFile.cpp GetInputFile.h header.h
	$(CC) $(CFLAGS) -c GetInputFile.cpp
	
Writer.o: Writer.cpp Item.h header.h
	$(CC) $(CFLAGS) -c Writer.cpp	
	
clean:
	$(RM) main *.o *~
