/**
 * Summary: Contains the main function for a simple shell program.
 * 
 * Current functionality: Runs the shell loop.
 * 
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 * file: mysh.c
 */

#include "mystring.h"
#include "jobs.h"
#include "myheap.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MAX_TOKENS 300
#define MAX_ARGS 10
char buffer[MAX_TOKENS]; // Global Buffer
static void print_command(const Command *cmd);
extern char **environ;
void run_command(const Command *cmd);


//Structures
struct Command cmd;

//Function prototypes
void get_command(Command *cmd);
void read_input(/*char *buffer, */int *input_length);
int check_exit(char *buffer, int *input_length);
void tokenize(/*char *buffer,*/ int *input_length, Command *cmd);


int main(int argc, char *argv[], char *envp[])
{
  int exitShell = 0;
  int input_length = 0;
  //char buffer[MAX_TOKENS];
  
  get_command(&cmd);  
  run_command(&cmd); 
  return 0;
  while (!exitShell)
    {
      /* TO DO: process command line */
      //this is where the parent child process stuff will go

      if (!exitShell) {
        //TESTING PURPOSES ONLY.............
        //echo input back to screen
        write(1, buffer, input_length);     
        if (input_length > 0){
          write(1, "\n", 1);
        }
        //..................................

        get_command(&cmd);
        }

    }
  
  return 0;
}

/**
 * @brief This function prompts the user for input, reads the input from
 *          the keyboard, and parses it into a Command structure.
 * 
 * @param cmd A pointer to a Command structure to store the parsed command.
 */
void get_command(Command *cmd) {
  //char buffer[MAX_TOKENS];
  int input_length = 0;

  write(1, "$ ", 2); //prompt
  read_input(/*buffer, */&input_length); //read command line into buffer

  tokenize(/*buffer,*/ &input_length, cmd); //parse input into arguments
  free_all();
  

  return;
}


/**
 * @brief This function prompts the user for input and then reads the input
 *          from the keyboard into the buffer.
 * 
 * @param buffer A character array to store the user input.
 * @param input_length A pointer to an integer to store the length of the input string.
 */
void read_input(/*char *buffer, */int *input_length) {
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
 * @brief This function parses the input buffer into arguments.
 * 
 * @param buffer A character array containing the user input.
 * @param input_length A pointer to an integer representing the length of the input.
 */
void tokenize(/*char *buffer,*/ int *input_length, Command *cmd) {
  // TO DO: parse the input buffer into arguments
  int start = 0;
  int end = 0;
  int token_length;

  // Skip over white space
  while (start < *input_length && (buffer[start] == ' ' || buffer[start] == '\t')) {
    start++;
  }
  //set input_length to 0 if input is all spaces or tabs
  if (start >= *input_length) {
    *input_length = 0; // No arguments
    cmd->argv[0] = NULL;
    return;
  }

  // loop for 
  while(start < *input_length){
    //find the next space or tab char
    end = start;
    while (end < *input_length && buffer[end] != ' ' && buffer[end] != '\t') {
    end++;
    }

    token_length = end - start;
    if (token_length > 0){  
      if (cmd->argc < MAX_ARGS){
        char *token = alloc(token_length+1);
        for(int i = 0; i<token_length; i++){
            token[i] = buffer[start + i];
        }
        token[token_length] = '\0';
        cmd->argv[cmd->argc++] = token;
      }
      else{
        write(2, "Too many args\n", 14);
        break;
      }
    }
      // Advance past this token
        start = end;

        
      while (start < *input_length && (buffer[start] == ' ' || buffer[start] == '\t')) {
          start++;
      
      }
  }
  
  cmd->argv[cmd->argc] = NULL;

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

static void print_command(const Command *cmd) {
    printf("argc=%d\n", cmd->argc);
    for (int i = 0; i < cmd->argc; i++) {
        printf("argv[%d] = \"%s\"\n", i, cmd->argv[i]);
    }
}

void run_command(const Command *cmd) {
    if (cmd->argc == 0) return;             // nothing to run

    pid_t pid = fork();
 
}