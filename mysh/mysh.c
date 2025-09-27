/**
 * Summary: Contains the main function for a simple shell program.
 * 
 * Current functionality: Runs the shell loop.
 * 
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 * file: bash.c
 */

#include "mystring.h"
#include "jobs.h"
#include <unistd.h>

#define MAX_TOKENS 300

//Function prototypes
void read_input(char *buffer);


int main(int argc, char *argv[], char *envp[])
{
  int exitShell = 0;
  char buffer[MAX_TOKENS];
  
  /* TO DO: prompt for and read command line */
  write(1, "$ ", 2); //prompt
  read_input(buffer); //read command line

  while (!exitShell)
    {
      //TESTING PURPOSES ONLY.............
      //echo input back to screen
      int i = 0;
      write(1, buffer, i);        
      if (i > 0){
        write(1, "\n", 1);
      }
      //..................................
      
      /* TO DO: process command line */
      /* TO DO: prompt for and read command line */
      write(1, "$ ", 2); //prompt
      read_input(buffer); //read command line
    }
  
  return 0;
}

/**
 * @brief This function prompts the user for input and then reads the input
 *          from the keyboard into the buffer.
 * 
 * @param buffer A character array to store the user input.
 */
void read_input(char *buffer) {
    char c;
    int i = 0, j, n;

    //loads input into buffer
    n = read(0, &c, 1); //reads 1 char from the input stream into c
    while (n > 0 && c != '\n' && i < MAX_TOKENS - 1) {
      buffer[i++] = c;
      n = read(0, &c, 1);
    }

    /*
    //check for input exceeding max char limit
    if (c != '\n') {
      // error message
      write(1, "Error: input exceeds max char limit\n", 37);

      //clear input until newline
      while (n > 0 && c != '\n') {
        n = read(0, &c, 1);
      }

      //skips the rest of the loop and starts over
    }*/ 
   return;
}
