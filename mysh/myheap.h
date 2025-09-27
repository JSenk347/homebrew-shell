#ifndef MY_HEAP_H
#define MY_HEAP_H

/**
* Provides a variable with the specified size from the heap
* @param size size of data chunk being allocated
* @return pointer
*/
char *alloc(unsigned int size);
void free_all();

#endif
