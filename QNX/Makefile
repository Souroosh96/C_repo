CC = gcc
CFLAGS = -Wall -g

all: myprogram

myprogram: myprogram.o
	$(CC) $(CFLAGS) -o myprogram myprogram.o

myprogram.o: Reader_Writer_Threads.c
	$(CC) $(CFLAGS) -c Reader_Writer_Threads.c -o myprogram.o

clean:
	rm -f myprogram myprogram.o