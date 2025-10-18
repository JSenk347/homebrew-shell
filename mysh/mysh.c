/**
 * Summary: Simple shell (mysh) "main" file that supports n-stage pipes (|), 
 *          redirection (<, >), and background (&) processes.
 * @attention Command format (simple rules):
 *   cmd args... [ '|' cmd args... ]... [ '<' infile ] [ '>' outfile ] [ '&' ]
 * - Words are split by spaces or tabs.
 * - Special symbols | < > & must have spaces around them.
 * - '<' and '>' apply to the whole pipeline.
 * - '&' must be last.
 *
 * Memory: uses myheap. We reset it BEFORE parsing a line and AFTER running it.
 * 
 * @authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 * 
 * @file mysh.c
 */
#define _POSIX_C_SOURCE 200809L   
#include "mysh.h"
#include "jobs.h"
#include "mystring.h"
#include "myheap.h"
#include "commands.h"
#include "run_job.h"
#include <signal.h>

char buffer[MAX_CHAR]; // input buffer

int isExit(const Command *cmd);
Job get_job();

int main(int argc, char *argv[], char *envp[]) {
  setup_signals();
  while (1) {
    Job job = get_job();
    if (job.num_stages == 0) continue;   // empty line
    if (isExit(&job.pipeline[0])) break; // exit/Exit/EXIT
    run_job(&job, envp);
    free_all();                          // free tokens from last run
  }
  return 0;
}

/**
 * @brief This function prompts the user for input, reads the input from
 *        the keyboard, creates a Job structure, and parses the input into
 *        the Job structure.
 *
 * @return Job A Job structure to store the parsed job.
 */
Job get_job() {
  prompt();

  int input_length = 0;
  int input_index  = 0;

  read_input(&input_length);
  free_all();                 // reset heap before parsing

  Job job;
  job.num_stages    = 0;
  job.infile_path   = NULL;
  job.outfile_path  = NULL;
  job.is_background = FALSE;

  parse_job(&job, &input_index);
  return job;
}

/**
 * @brief Checks for the "exit" command and returns 1 if found, otherwise 0.
 *
 * @param cmd A pointer to the Command structure to check.
 * @return int Returns 1 if the command is "exit", otherwise returns 0.
 */
int isExit(const Command *cmd) {
  if (cmd->argc == 1 && mystrlen(cmd->argv[0]) == 4) {
    if (mystrcmp(cmd->argv[0], "exit") == 0) return 1;
    if (mystrcmp(cmd->argv[0], "EXIT") == 0) return 1;
    if (mystrcmp(cmd->argv[0], "Exit") == 0) return 1;
  }
  return 0;
}

/**
* @brief @todo by joseeeeeeeeph
*
*/
void setup_signals() {
  struct sigaction ign;
  sigemptyset(&ign.sa_mask);
  ign.sa_handler = SIG_IGN;
#ifdef SA_RESTART
  ign.sa_flags = SA_RESTART;   // auto-restart read()/waitpid() etc. if interrupted
#endif
  sigaction(SIGINT,  &ign, NULL);   // ignore Ctrl-C in the shell
  sigaction(SIGQUIT, &ign, NULL);   // ignore Ctrl-\ in the shell

  // OPTION A (recommended): reap background children as soon as they exit
  struct sigaction chld;
  sigemptyset(&chld.sa_mask);
  chld.sa_handler = soul_reaper;
#ifdef SA_RESTART
  chld.sa_flags = SA_RESTART;
#endif
  sigaction(SIGCHLD, &chld, NULL);
}

/**
* @brief Reap finished background children without blocking the parent.
*/
void soul_reaper() {
  int status;
  while (waitpid(-1, &status, WNOHANG) > 0);
}
