/**
 * Summary: Contains the main function for a simple shell program.
 *
 * Current functionality: Runs the shell loop - prompts user for command,
 *    can run jobs with single stages/commands (no pipes, redirection, or
 *    background)
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
#include <sys/stat.h>
#include <fcntl.h>
#define MAX_TOKENS 300
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
void read_input(int *input_length);
void parse_job(Job *job, int *input_index);
void parse_pipeline(Job *job, int *input_index);
void parse_command(Command *cmd, int *input_index, int *input_length);
void add_arg(Command *cmd, int token_length, int *curr);
void read_filename(Job *job, int *input_index, int *input_length, char type); //NEW
void find_start(int *start, int *input_length);
void find_tkn_end(int *start, int *end, int *input_length);
int isExit(const Command *cmd);
void run_command(Command *cmd, char *envp[]);
void run_job(Job *job, char *envp[]);
// Command get_command(); // WILL BE REPLACED BY GET_JOB()
pid_t execute_command(Command *cmd, char *envp[], int fd_in, int fd_out);
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

  while (!isExit(/**&cmd*/ &job.pipeline[0]))
  {
    // run_job(&job);
    run_command(&job.pipeline[0], envp); // will be replaced by run_job()
    job = get_job();
  }
  return 0;
}

// INPUT READ/STORE FUNCTIONS
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
  parse_job(job_ptr, &input_index); // parse buffer into job
  free_all();

  return job;
}

/**
 * @brief This function reads the input from the keyboard into the global buffer.
 *
 * @param input_length A pointer to an integer to store the length of the input
 *        string.
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
 * @brief This function parses the buffer which holds the full command line
 *        into a Job so the Job can tell the shell how many commands to run,
 *        what each command's arguments are, whether to connect commands with
 *        pipes, where to send input/output, and whether to run the command in
 *        the background.
 *
 * @param job A pointer to a Job structure which stores the parsed job.
 * @param input_index A pointer to an integer representing the current index in
 *        the input buffer.
 */
void parse_job(Job *job, int *input_index)
{
  int input_length = mystrlen(buffer);

  // populates job structure with commands and/or pipelines
  parse_pipeline(job, input_index);

  //NEW////////////////////////////
  // handle input/output redirection and background
  if (buffer[*input_index] == '<')
  {
    // input redirection
    input_index++; // move over '<'
    read_filename(job, input_index, &input_length, '<'); //updates job->infile_path
    return;
  }
  else if (buffer[*input_index] == '>')
  {
    // output redirection
    input_index++; // move over '>'
    read_filename(job, input_index, &input_length, '>'); //updates job->outfile_path
    return;
  }
  else if (buffer[*input_index] == '&')
  {
    input_index++; // move over '&'
    job->is_background = TRUE;
  }
  //////////////////////////////
  return;
}

/**
 * @brief Builds a sequence of command structures representing a pipeline of
 *        commands as long as the next symbol after each command is the char
 *        literal '|'.
 *
 * @param job A pointer to a Job structure to store the parsed pipeline
 *        commands.
 * @param input_index A pointer to an integer representing the current index in
 *        the input buffer.
 */
void parse_pipeline(Job *job, int *input_index)
{
  Command cmd;
  int input_length = mystrlen(buffer); // length of full input

  // parses input into tokens for first command
  parse_command(&cmd, input_index, &input_length);
  job->pipeline[job->num_stages] = cmd; // add first command to job
  job->num_stages++;

  while (buffer[*input_index] == '|')
  {
    // continue parsing next command in pipeline
    (*input_index)++; // move over '|'
    parse_command(&cmd, input_index, &input_length);
    job->pipeline[job->num_stages] = cmd; // add next command to job
    job->num_stages++;
  }

  return;
}

/**
 * @brief This function parses the input buffer into commands by extracting
 *        each token until either one of the char literals '<', '>', '|', or
 *        '&', or the end of line is reached.
 *
 * @param cmd A pointer to a Command structure to store the parsed arguments.
 * @param input_index A pointer to an integer representing the current index in
 *        the input buffer.
 * @param input_length A pointer to an integer representing the length of the
 *        command line imput.
 */
void parse_command(Command *cmd, int *input_index, int *input_length)
{
  cmd->argc = 0;
  int curr = *input_index /**0*/;
  int end = *input_index /**0*/;
  int token_len = 0;

  // adjusts start to be 0th index of next arg (if any)
  find_start(&curr, input_length);

  // checks if input is entirely whitespace
  if (curr >= *input_length)
  {
    *input_length = 0; // No arguments
    cmd->argv[0] = NULL;
    return;
  }

  // adds each token to the cmd struct as an argument
  while (curr < *input_length)
  {
    // find the next space or tab char
    find_tkn_end(&curr, &end, input_length);

    // Check if the token is any of the special literals and handle accordingly
    if (end - curr == 1 && (mystrcmp(&buffer[curr], "|") == 0 ||
                            mystrcmp(&buffer[curr], "<") == 0 ||
                            mystrcmp(&buffer[curr], ">") == 0 ||
                            mystrcmp(&buffer[curr], "&") == 0))
    {

      *input_index = curr; // updates input_index to the current position in buffer
      return;              // return to parse_pipeline
    }

    // adds the token to the cmd struct as an argument
    int token_length = end - curr;
    add_arg(cmd, token_length, &curr);

    // move to end of added token
    curr = end;
    *input_index = curr;

    // skip over spaces and tabs to the start of the next token (if any)
    find_start(&curr, input_length);
  }

  cmd->argv[cmd->argc] = NULL;
  *input_index = curr; // updates input_index to the current position in buffer

  return;
}

 /**
  * @brief Adds a token of specified length from the input buffer to the command's
  *        argument list if the token length is greater than zero and the maximum
  *        number of arguments has not been exceeded.
  * @param cmd A pointer to a Command structure to store the parsed arguments.
  * @param token_length The length of the token to add.
  * @param curr A pointer to the current index in the input buffer.
  */
void add_arg(Command *cmd, int token_length, int *curr)
{
  if (token_length > 0)
  {
    if (cmd->argc < MAX_ARGS)
    {
      char *token = alloc(token_length + 1);
      for (int i = 0; i < token_length; i++)
      {
        token[i] = buffer[*curr + i];
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

//NEW/////////////////////
/**
 * @brief This function reads the next token from the input buffer
 *       and assigns it to the appropriate file path in the Job structure.
 * 
 * @param job A pointer to a Job structure to store the parsed file path.
 * @param input_index A pointer to an integer representing the current index in
 *        the input buffer.
 * @param input_length A pointer to an integer representing the length of the
 *        command line imput.
 * @param type A char indicating whether the filename is for input ('<')
 *        or output ('>') redirection.  
 */
void read_filename(Job *job, int *input_index, int *input_length, char type) {
  int start;
  int end;
  int fname_length;
  char *filename;

  find_start(input_index, input_length); // skips any leading whitespace
  start = *input_index;

  find_tkn_end(input_index, &end, input_length); // finds the end of the filename token

  fname_length = end - start;
  filename = alloc(fname_length + 1); // allocate memory for filename
  for (int i = 0; i < fname_length; i++) {
    filename[i] = buffer[start + i];
  }

  if (type == '<') {
    job->infile_path = filename; // set input file path
  } else if (type == '>') {
    job->outfile_path = filename; // set output file path
  }
return;
}
////////////////////////

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
 * @brief Executes a Job, handling I/O redirection and backgrounding.
 * Currently supports single-stage jobs only.
 *
 * @param job  Job structure containing the pipeline and metadata.
 * @param envp Environment passed down from main.
 */
void run_job(Job *job, char *envp[])
{
  // Only handling single-stage jobs for now
  if (job->num_stages != 1)
  {
    write(2, "Pipeline or empty job not yet supported.\n", 41);
    return;
  }

  Command *cmd = &job->pipeline[0];
  int input_fd = 0;
  int output_fd = 1;
  int pid;

  // redirecting input
  if (job->infile_path != NULL)
  {
    input_fd = open(job->infile_path, O_RDONLY);
    if (input_fd == -1)
    {
      write(2, "error opening input file\n", 25);
      return; 
    }
  }
  // redirecting output
  if (job->outfile_path != NULL)
  {
    // O_TRUNC clears the file, 0644 are standard permissions
    output_fd = open(job->outfile_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (output_fd == -1)
    {
      write(2, "error opening output file\n", 25);
      if (input_fd != 0)
        close(input_fd);
      return; 
    }
  }
  // execute the single command
  pid = execute_command(cmd, envp, input_fd, output_fd);

  // Close file descriptors opened in the parent
  if (input_fd != 0)
    close(input_fd);
  if (output_fd != 1)
    close(output_fd);

  // b. Waiting (Background vs. Foreground)
  if (pid != -1)
  {
    if (job->is_background == TRUE)
    {
      // Background job: Parent returns immediately
      // You would typically track this job here.
      printf("mysh: Background PID [%d]\n", pid);
    }
    else
    {
      // Foreground job: Parent must wait
      int status;
      waitpid(pid, &status, 0);
    }
  }
}

pid_t execute_command(Command *cmd, char *envp[], int fd_in, int fd_out)
{
  char base_path[] = "/usr/bin/";

  if (cmd->argc == 0)
  {
    return -1; // No command to execute
  }

  pid_t pid = fork();
  if (pid < 0)
  {
    // error
    write(2, "fork failed\n", 12);
    return -1;
  }

  if (pid == 0)
  {
    // Child doing some work
    // Set up I/O redirection for this stage
    if (fd_in != 0)
    {
      dup2(fd_in, 0);
      close(fd_in);
    }
    if (fd_out != 1)
    {
      dup2(fd_out, 1);
      close(fd_out);
    }

    // Prepare and execute the command
    cmd->argv[cmd->argc] = NULL;                         // ensure argv is null terminated
    char *full_path = mystrcat(cmd->argv[0], base_path); // appending argument 0 to base path "/user/bin/"

    execve(full_path, cmd->argv, envp);

    // execve only returns on error
    write(2, "execve failed\n", 14);
    _exit(1); // Exit child process on execution failure
  }

  // Parent returns the PID
  return pid;
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

// DEBUGGING/TESTING FUNCTIONS
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

// GRAVEYARD OF CURRENTLY UNUSED FUNCTIONS
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