/**
 * Summary: Contains the main function for a simple shell program.
 *
 * Current functionality: Runs the shell loop - prompts user for command,
 *    execve fails every time, exit works.
 *
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 * @file: mysh.c
 */

#include "mystring.h"
#include "jobs.h"
#include "myheap.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define MAX_TOKENS 300
#define MAX_ARGS 10
#define MAX_JOBS 10
#define MAX_COMMANDS 10

char buffer[MAX_TOKENS]; // Global Buffer
static void print_command(const Command *cmd);
Command commands[MAX_COMMANDS];
Job all_jobs[MAX_JOBS]; // Array to hold all jobs;

// STRUCTURES
struct Command cmd;
struct Job job1;


// FUNCTION PROTOTYPES
void get_command(Command *cmd);
void read_input(/*char *buffer, */ int *input_length);
void parse_pipeline(int *input_length);
Command parse_command(int *input_length, char *delimiter);
void add_arg(Command *cmd, int token_length, int *start_ptr); /*trying this out ARR*/
void tokenize(/*char *buffer,*/ int *input_length, Command *cmd);
void find_start(int *start, int *input_length);
void find_tkn_end(int *start, int *end, int *input_length);
int isExit(const Command *cmd);
void run_command(const Command *cmd, char *envp[]);
// int check_exit(char *buffer, int *input_length); not in use - ARR

// TEST INSTANCES OF JOB/////////////////////////
struct Job eg1 = {
  {{{"ls", "-al", "/library", NULL}, 3}},
  1,
  NULL,
  NULL,
  0
};
struct Job eg2 ={
  {{{"ls", "-al", "/library", NULL}, 3},
  {{"wc", "-l", NULL}, 2}},
  2,
  "dirlistlen",
  NULL,
  1
};
/////////////////////////////////////////////////

int main(int argc, char *argv[], char *envp[])
{
  get_command(&cmd);
  {
    run_command(&cmd, envp);
    get_command(&cmd);
  }
  return 0;
}


// HELPER FUNCTIONS
/**
 * @brief To do
 *
 * @param job A pointer to a Job structure to store the job details.
*/
void get_job(Job *job){
  // get an entire job and invoke run_job
  get_command(&cmd);
  //populate_job
}

/**
 * @brief This function prompts the user for input, reads the input from
 *          the keyboard, and parses it into a Command structure.
 *
 * @param cmd A pointer to a Command structure to store the parsed command.
 */
void get_command(Command *cmd)
{
  // char buffer[MAX_TOKENS];
  int input_length = 0;

  write(1, "$ ", 2);                      // prompt
  
  read_input(/*buffer, */ &input_length); // read command line into buffer
  free_all();
  *cmd = parse_command(&input_length, "");
  /**tokenize(/*buffer, &input_length, cmd); // parse input into arguments*/
  free_all();

  return;
}


/**
 * @brief This function prompts the user for input and then reads the input
 *          from the keyboard into the buffer.
 *
 * @param input_length A pointer to an integer to store the length of the input string.
 */
void read_input(int *input_length)
{
  char c;
  int i = 0;
  int n;

  // loads input into buffer
  n = read(0, &c, 1); // reads 1 char from the input stream into c
  while (n > 0 && c != '\n' && i < MAX_TOKENS - 1)
  {
    buffer[i++] = c;
    n = read(0, &c, 1);
  }

  buffer[i] = '\0';
  *input_length = i;
  return;
}

/**
 * @brief This function parses the
 *
 * @param input_length A pointer to an integer representing the length of the command line input.
 */
void parse_pipeline(int *input_length){  
  struct Command write_cmd;
  struct Command read_cmd;
  char delimiter;

  write_cmd = parse_command(input_length, &delimiter);
  if (delimiter == '|')
  {
    read_cmd = parse_command(input_length, &delimiter);
  }
}

/**
 * @brief This function parses the input buffer into commands.
 *
 * @param input_length A pointer to an integer representing the length of the command line imput.
 * @return Command
 */
Command parse_command(int *input_length, char *delimiter)
{
  struct Command cmd;
  Command *cmd_ptr = &cmd;
  cmd_ptr -> argc = 0;
  int curr, end, token_len = 0;
  int *start_ptr = &curr;
  int *end_ptr = &end;

  find_start(start_ptr, input_length); // adjusts start to be 0th index of first arg (if any)
  
  // set input_length to 0 if input is all spaces or tabs
  if (curr >= *input_length)
  {
    *input_length = 0; // No arguments
    cmd_ptr->argv[0] = NULL;
    return *cmd_ptr; // just doing this for now to avoid warnings
  }

  // adds each token to the cmd struct as an argument
  while (curr < *input_length)
  {
    // find the next space or tab char
    find_tkn_end(start_ptr, end_ptr, input_length);

    // Check if the token is '|' and handle accordingly
    if (end - curr == 1 && (mystrcmp(&buffer[curr], "|") || mystrcmp(&buffer[curr], "<") ||
                            mystrcmp(&buffer[curr], ">") || (mystrcmp(&buffer[curr], "&"))))
    {
      *delimiter = buffer[curr];
      return *cmd_ptr; // return to parse_pipeline
    }

    // adds the token to the cmd struct as an argument
    int token_length = end - curr;
    add_arg(cmd_ptr, token_length, start_ptr);
    
    // move to end of added token
    curr = end;

    // skip over spaces and tabs to the start of the next token (if any)
    find_start(start_ptr, input_length);
  }

  return *cmd_ptr;
}

void add_arg(Command *cmd, int token_length, int *start_ptr){
  if (token_length > 0)
    {
      printf("cmd->argc is: %d\n", cmd->argc); //debugging
      if (cmd->argc < MAX_ARGS)
      {
        char *token = alloc(token_length + 1);
        for (int i = 0; i < token_length; i++)
        {
          token[i] = buffer[*start_ptr + i];
        }
        token[token_length] = '\0';
        cmd->argv[cmd->argc++] = token;
      }
      else
      {
        write(2, "Too many args\n", 14);
      }
    }
}
 
/**
 * @brief This function parses the input buffer into arguments.
 *
 * @param input_length A pointer to an integer representing the length of the input.
 * @param cmd A pointer to a Command structure to store the parsed arguments.
 */
void tokenize(int *input_length, Command *cmd)
{
  int start = 0;
  int end = 0;
  int token_length;
  cmd->argc = 0;
  int *start_ptr = &start;
  int *end_ptr = &end;
 
  find_start(start_ptr, input_length);

  // set input_length to 0 if input is all spaces or tabs
  if (start >= *input_length)
  {
    *input_length = 0; // No arguments
    cmd->argv[0] = NULL;
    return;
  }

  // Main tokenizing loop
  while (start < *input_length)
  {
    // find the next space or tab char
    find_tkn_end(start_ptr, end_ptr, input_length);

    token_length = end - start;
    add_arg(cmd, token_length, start_ptr);
    /**if (token_length > 0)
    {
      if (cmd->argc < MAX_ARGS)
      {
        char *token = alloc(token_length + 1);
        for (int i = 0; i < token_length; i++)
        {
          token[i] = buffer[start + i];
        }
        token[token_length] = '\0';
        cmd->argv[cmd->argc++] = token;
      }
      else
      {
        write(2, "Too many args\n", 14);
        break;
      }
    }*/
    // Advance past this token
    start = end;

    while (start < *input_length && (buffer[start] == ' ' || buffer[start] == '\t'))
    {
      start++;
    }
  }

  cmd->argv[cmd->argc] = NULL;

  return;
}

/**
 * @brief Skip over initial white space until a non-white space char is found.
 *
 * @param start Pointer to the starting index to modify.
 * @param input_length The total length of the input buffer.
 */
void find_start(int *start, int *input_length)
{
  while (*start < *input_length && (buffer[*start] == ' ' || buffer[*start] == '\t'))
  {
    (*start)++;
  }
}

/**
 * @brief Find the end index of the current token by searching for the next
 *        white space character or the end of the input.
 *
 * @param start Pointer to the starting index of the token.
 * @param end Pointer to the ending index of the token to modify.
 * @param input_length The total length of the input buffer.
 */
void find_tkn_end(int *start, int *end, int *input_length)
{
  *end = *start;
  while (*end < *input_length && buffer[*end] != ' ' && buffer[*end] != '\t')
  {
    (*end)++;
  }
}

/**
 * @brief Checks for the "exit" command and returns 1 if found, otherwise 0.
 *
 * @param cmd A pointer to the Command structure to check.
 * @return int Returns 1 if the command is "exit", otherwise returns 0.
 */
int isExit(const Command *cmd)
{
  if (cmd->argc == 1 && mystrlen(cmd->argv[0]) == 4)
  {
    if (mystrcmp(cmd->argv[0], "exit") == 0)
    {
      return 1;
    }
    else if (mystrcmp(cmd->argv[0], "EXIT") == 0)
    {
      return 1;
    }
    else if (mystrcmp(cmd->argv[0], "Exit") == 0)
    {
      return 1;
    }
  }
  return 0;
}

/**
 * @brief This function prints the contents of a Command structure for
 *        debugging purposes.
 *
 * @param cmd A pointer to the Command structure to print
 */
static void print_command(const Command *cmd)
{
  printf("argc=%d\n", cmd->argc);
  for (int i = 0; i < cmd->argc; i++)
  {
    printf("argv[%d] = \"%s\"\n", i, cmd->argv[i]);
  }
}

/**
 * @brief Execute a command by forking to create a child and running execve.
 *
 * @param cmd  Parsed command structure (argv/argc).
 * @param envp Environment passed down from main.
 */
void run_command(const Command *cmd, char *envp[])
{
  char base_path[] = "/usr/bin/";

  if (cmd->argc == 0)
    return; // no commands were sent

  /** shouldn't this be happening in the main while loop?? */
  // exit
  /**if (mystrcmp(cmd->argv[0], "exit") == 0)
  {
    _exit(0);
  }*/

  pid_t pid = fork();
  if (pid < 0)
  {
    // error
    write(2, "fork failed\n", 12);
    _exit(0);
  }

  if (pid == 0)
  {
    // Child doing some work
    char *full_path = mystrcat(cmd->argv[0], base_path); // appending argument 0 to base path "/user/bin/"
    execve(full_path, cmd->argv, envp);

    // error
    write(2, "execve failed\n", 14);
    _exit(0);
  }
  // stop parent from working until the child is gone
  int status;
  waitpid(pid, &status, 0);
}

// CURRENTLY UNUSED FUNCTIONS
/**
 * @brief This function checks if the input command is "exit" (case-insensitive).
 *
 * @param buffer A character array containing the user input.
 * @param input_length A pointer to an integer representing the length of the input.
 *
 * @return int Returns 1 if the input is "exit", otherwise returns 0.

int check_exit(char *buffer, int *input_length) {
  if (*input_length == 4 && (buffer[0] == 'e' || buffer[0] == 'E') &&
    (buffer[1] == 'x' || buffer[1] == 'X') &&
    (buffer[2] == 'i' || buffer[2] == 'I') &&
    (buffer[3] == 't' || buffer[3] == 'T')) {
    return 1;
  }

  return 0;
}
 */