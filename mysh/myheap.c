#include <unistd.h>
#include <stdio.h>
#include "myheap.h"

#define HEAP_SIZE 10000     /* adjust as necessary */
#define HEAP_OVERFLOW_ERR "You have run out of available space on the heap bitch!!!\n"

static char heap[HEAP_SIZE]; //Big 'Ol healp
static char *freep = heap; //Pointer to next available chunk of memory on the heap

/*size is the number of cahracters to put on the buffer*/
char *alloc(unsigned int size)
{
  if (size == 0 || freep + size > heap + HEAP_SIZE){
    write(1, &HEAP_OVERFLOW_ERR, 57);
    _exit(0);
  }

  char *ptr = freep;
  freep += size;
  return ptr;     
}

void free_all()
{
  freep = heap;
}

