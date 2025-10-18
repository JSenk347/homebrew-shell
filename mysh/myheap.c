/**
 * Summary: Contains heap memory allocation functions.
 * 
 * Current functionality: Implements a simple heap allocator with alloc and
 *    free_all functions.
 * 
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 * 
 * @file: myheap.c
 */
#include <unistd.h>
#include <stdio.h>
#include "myheap.h"

#define HEAP_SIZE 10000     
#define HEAP_OVERFLOW_ERR "You have run out of available space on the heap\n"

static char heap[HEAP_SIZE]; //Big Ol' heap
static char *freep = heap; //Ptr to next available chunk of memory on the heap

/**
* @brief Provides a variable with the specified size from the heap where size
*        is the number of characters to put on the buffer.
*
* @param size size of data chunk being allocated
* @return pointer
*/
char *alloc(unsigned int size)
{
  if (size == 0 || freep + size > heap + HEAP_SIZE){
    write(1, &HEAP_OVERFLOW_ERR, sizeof(HEAP_OVERFLOW_ERR) - 1);
    _exit(0);
  }

  char *ptr = freep;
  freep += size;
  return ptr;
}

/**
 * @brief Frees the heap by pointing to freep
 *        pointer to the front of the array
 */
void free_all()
{
  freep = heap;
}