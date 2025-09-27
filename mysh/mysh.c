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
void read_input(char *buffer, int *input_length);
int check_exit(char *buffer, int *input_length);


int main(int argc, char *argv[], char *envp[])
{
  int exitShell = 0;
  int input_length = 0;
  char buffer[MAX_TOKENS];
  
  /* TO DO: prompt for and read command line */
  write(1, "$ ", 2); //prompt
  read_input(buffer, &input_length); //read command line

  while (!exitShell)
    {
      /* TO DO: process command line */
      exitShell = check_exit(buffer, &input_length);

      if (!exitShell) {
        //TESTING PURPOSES ONLY.............
        //echo input back to screen
        write(1, buffer, input_length);     
        if (input_length > 0){
          write(1, "\n", 1);
        }
        //..................................

        /* TO DO: prompt for and read command line */
        write(1, "$ ", 2); //prompt
        read_input(buffer, &input_length); //read command line
        }

    }
  
  return 0;
}

/**
 * @brief This function prompts the user for input and then reads the input
 *          from the keyboard into the buffer.
 * 
 * @param buffer A character array to store the user input.
 * @param input_length A pointer to an integer to store the length of the input.
 */
void read_input(char *buffer, int *input_length) {
  char c;
  int i = 0;
  int n;

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

  *input_length = i;
  return;
}

/**
 * @brief This function checks if the input command is "exit" (case-insensitive).
 * 
 * @param buffer A character array containing the user input.
 * @param input_length A pointer to an integer representing the length of the input.
 * 
 * @return int Returns 1 if the input is "exit", otherwise returns 0.
 */
int check_exit(char *buffer, int *input_length) {
  if (*input_length == 4 && (buffer[0] == 'e' || buffer[0] == 'E') &&
    (buffer[1] == 'x' || buffer[1] == 'X') &&
    (buffer[2] == 'i' || buffer[2] == 'I') &&
    (buffer[3] == 't' || buffer[3] == 'T')) {
    return 1;
  }

  return 0;
}
