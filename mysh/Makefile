mysh: mysh.o mystring.o myheap.o
	gcc mysh.o mystring.o myheap.o -o mysh

mysh.o: mysh.c mystring.h jobs.h
	gcc -c mysh.c

mystring.o: mystring.c mystring.h
	gcc -c mystring.c

myheap.o: myheap.c myheap.h
	gcc -c myheap.c

clean:
	/usr/bin/rm -f *.o mysh

all: clean mysh
