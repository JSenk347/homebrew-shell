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
#define MAX_CHAR 300
#define MAX_ARGS 10
#define MAX_JOBS 10
#define MAX_COMMANDS 10
#define TRUE 1
#define FALSE 0

char buffer[MAX_CHAR]; // Global Buffer
static void print_command(const Command *cmd);
Command commands[MAX_COMMANDS]; // array of commands
Job all_jobs[MAX_JOBS];         // Array to hold all jobs;

// STRUCTURES
struct Job job1;

// FUNCTION PROTOTYPES
Job get_job();
void read_input(/*char *buffer, */ int *input_length);
void parse_job(Job *job, int *input_index);
void parse_pipeline(Job *job, int *input_index);
void parse_command(Command *cmd, int *input_index, int *input_length /**, char *delimiter*/);
void add_arg(Command *cmd, int token_length, int /***start_ptr*/*curr); /*trying this out ARR*/
void find_start(int *start, int *input_length);
void find_tkn_end(int *start, int *end, int *input_length);
int isExit(const Command *cmd);
void run_command(Command *cmd, char *envp[]);
// Command get_command(); // WILL BE REPLACED BY GET_JOB()
// void tokenize(/*char *buffer,*/ int *input_length, Command *cmd); - ARR
// int check_exit(char *buffer, int *input_length); not in use - ARR

// TEST INSTANCES OF JOB/////////////////////////
struct Job eg1 = {
    {{{"ls", "-al", "/library", NULL}, 3}},
    1,
    NULL,
    NULL,
    0};
struct Job eg2 = {
    {{{"ls", "-al", "/library", NULL}, 3},
     {{"wc", "-l", NULL}, 2}},
    2,
    "dirlistlen",
    NULL,
    1};
/////////////////////////////////////////////////

int main(int argc, char *argv[], char *envp[])
{
  Job job = get_job();
  // Command cmd = get_command(); // replace by get_job()

  while (!isExit(/**&cmd*/ &job.pipeline[0]))
  {
    // run_job(&job);
    run_command(&job.pipeline[0], envp); // will be replaced by run_job()
    job = get_job();
    // cmd = get_command();     // will be replaced by get_job()
  }
  return 0;
}

// INPUT PARSING HELPER FUNCTIONS
/**
 * @brief This function prompts the user for input, reads the input from
 *        the keyboard, creates a Job structure, and parses the input into
 *        the Job structure.
 *
 * @return Job A Job structure to store the parsed job.
 */
Job get_job()
{
  // initialize job
  Job job;
  job.num_stages = 0;
  job.infile_path = NULL;
  job.outfile_path = NULL;
  job.is_background = FALSE;

  Job *job_ptr = &job;
  int input_length = 0;
  int input_index = 0;

  write(1, "$ ", 2);         // prompt
  read_input(&input_length); // read command line into buffer
  free_all();
  parse_job(job_ptr, &input_index);
  free_all();

  return job;
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
  while (n > 0 && c != '\n' && i < MAX_CHAR - 1)
  {
    buffer[i++] = c;
    n = read(0, &c, 1);
  }

  buffer[i] = '\0';
  *input_length = i;
  return;
}

/**
 * @brief This function parses the full command line into a Job structure which
 *        can tell the shell how many commands to run, what each command's
 *        arguments are, whether to connect commands with pipes, where to send
 *        input/output, and whether to run in the background.
 *
 * @param job A pointer to a Job structure to store the parsed job.
 * @param input_index A pointer to an integer representing the current index in the input buffer.
 */
void parse_job(Job *job, int *input_index)
{
  int input_length = mystrlen(buffer);

  // populates job structure with commands and/or pipelines
  parse_pipeline(job, input_index);

  /** will need to be built to handle input/output redirection and background
  while (input_index < mystrlen(buffer))
  {
    if (buffer[input_index] == '<')
    {
      // input redirection
      input_index++;
      job.infile_path =
    }
    else if (buffer[input_index] == '>')
    {
      // output redirection
      input_index++;
      job.outfile_path =
    }
    else if (buffer[input_index] == '&')
    {
      input_index++;
      job.is_background = TRUE;
    }
    else
    {
      input_index++;
    }
  }*/

  return;
}

/**
 * @brief Builds a sequence of command structures representing a pipeline of
 *        commands as long as the next symbol after each command is the char
 *        literal '|'.
 *
 * @param job A pointer to a Job structure to store the parsed pipeline commands.
 * @param input_index A pointer to an integer representing the current index in the input buffer.
 */
void parse_pipeline(Job *job, int *input_index)
{
  Command cmd;
  int input_length = mystrlen(buffer) /** - *input_index*/; // length of full input
  // char delimiter = FALSE;

  // parses input into tokens for first command
  parse_command(&cmd, input_index, &input_length /**, &delimiter*/);
  job->pipeline[job->num_stages] = cmd;        // add first command to job
  job->num_stages++;

  while (buffer[*input_index] == '|')
  {
    // continue parsing next command in pipeline
    (*input_index)++; // skip over '|'
    parse_command(&cmd, input_index, &input_length /** , &delimiter*/);
    job->pipeline[job->num_stages] = cmd; // add next command to job
    job->num_stages++;
  }

  return;
}

void run_job(Job *job)
{
}

/**
 * @brief This function parses the input buffer into commands by extracting
 *        each token until either one of the char literals '<', '>', '|', or
 *        '&', or the end of line is reached.
 *
 * @param cmd A pointer to a Command structure to store the parsed arguments.
 * @param input_index A pointer to an integer representing the current index in the input buffer.
 * @param input_length A pointer to an integer representing the length of the command line imput.
 */
void parse_command(Command *cmd, int *input_index, int *input_length /**, char *delimiter*/)
{
  cmd->argc = 0;
  int curr = *input_index /**0*/;
  int end = *input_index /**0*/;
  int token_len = 0;
  //int *start_ptr = &curr;
  int *end_ptr = &end;
  
  find_start(/**start_ptr*/&curr, input_length); // adjusts start to be 0th index of next arg (if any)

  // set input_length to 0 if input is all spaces or tabs
  if (curr >= *input_length)
  {
    *input_length = 0; // No arguments
    cmd->argv[0] = NULL;
    return; // just doing this for now to avoid warnings
  }

  // adds each token to the cmd struct as an argument
  while (curr < *input_length)
  {
    // find the next space or tab char
    find_tkn_end(/**start_ptr*/&curr, end_ptr, input_length);

    // Check if the token is '|' and handle accordingly
    if (end - curr == 1 && (mystrcmp(&buffer[curr], "|") == 0 || mystrcmp(&buffer[curr], "<") == 0 ||
                            mystrcmp(&buffer[curr], ">") == 0 || mystrcmp(&buffer[curr], "&") == 0))
    {
      //*delimiter = buffer[curr];
      *input_index = curr; // updates input_index to the current position in buffer
      return;              // return to parse_pipeline
    }

    // adds the token to the cmd struct as an argument
    int token_length = end - curr;
    add_arg(cmd, token_length, /**start_ptr*/&curr);

    // move to end of added token
    curr = end;
    *input_index = curr;

    // skip over spaces and tabs to the start of the next token (if any)
    find_start(/**start_ptr*/ &curr, input_length);
  }

  cmd->argv[cmd->argc] = NULL;
  *input_index = curr; // updates input_index to the current position in buffer

  return;
}

void add_arg(Command *cmd, int token_length, int /***start_ptr*/*curr)
{  
  if (token_length > 0)
  {
    if (cmd->argc < MAX_ARGS)
    {
      char *token = alloc(token_length + 1);
      for (int i = 0; i < token_length; i++)
      {
        token[i] = buffer[/***start_ptr*/*curr + i];
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
void run_command(Command *cmd, char *envp[])
{
  char base_path[] = "/usr/bin/";

  if (cmd->argc == 0)
  {
    return; // no commands were sent
  }

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
    cmd->argv[cmd->argc] = NULL;                         // ensure argv is NULL-terminated
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
 * @brief This function parses the input buffer into arguments.
 *
 * @param input_length A pointer to an integer representing the length of the input.
 * @param cmd A pointer to a Command structure to store the parsed arguments.
 */
/**void tokenize(int *input_length, Command *cmd)
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
    }/
    // Advance past this token
    start = end;

    while (start < *input_length && (buffer[start] == ' ' || buffer[start] == '\t'))
    {
      start++;
    }
  }

  cmd->argv[cmd->argc] = NULL;

  return;
}*/

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

/** REPLACED WITH GET_JOB()
 * @brief This function prompts the user for input, reads the input from
 *        the keyboard, creates a command structure, and parses it into
 *        the Command structure.
 *
 * @param cmd A pointer to a Command structure to store the parsed command.
 */
/**Command get_command()
{
  Command cmd;
  Command *cmd_ptr = &cmd;
  int input_length = 0;

  write(1, "$ ", 2); // prompt

  read_input(&input_length); // read command line into buffer
  free_all();
  parse_command(cmd_ptr, &input_length, "");
  //tokenize(/*buffer, &input_length, cmd); // parse input into arguments
  free_all();

  return cmd;
}*/