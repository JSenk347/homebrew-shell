mysh: mysh.o mystring.o myheap.o commands.o
	gcc mysh.o mystring.o myheap.o commands.o -o mysh

mysh.o: mysh.c mystring.h jobs.h mysh.h commands.h
	gcc -c mysh.c

mystring.o: mystring.c mystring.h
	gcc -c mystring.c

myheap.o: myheap.c myheap.h
	gcc -c myheap.c

commands.o: commands.c commands.h mysh.h jobs.h mystring.h myheap.h
	gcc -c commands.c

clean:
	/usr/bin/rm -f *.o mysh

all: clean mysh
