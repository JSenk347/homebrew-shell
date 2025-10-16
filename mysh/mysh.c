// path: src/mysh.c
/**
 * Summary: Simple shell (mysh) that supports pipes (|), redirection (<, >),
 *          and background (&).
 *
 * Command format (simple rules):
 *   cmd args... [ '|' cmd args... ]... [ '<' infile ] [ '>' outfile ] [ '&' ]
 * - Words are split by spaces or tabs.
 * - Special symbols | < > & must have spaces around them.
 * - '<' and '>' apply to the whole pipeline.
 * - '&' must be last.
 *
 * Memory: uses myheap. We reset it BEFORE parsing a line and AFTER running it.
 *
 * Authors: Anna Running Rabbit, Joseph Mills, Jordan Senko
 */

#include "mysh.h"
#include "jobs.h"
#include "string.h"
#include "myheap.h"
#include "commands.h"
// Globals
char buffer[MAX_CHAR];  // input buffer

int  isExit(const Command *cmd);
Job  get_job();

// Small helpers (implementations)

/**
 * @brief Write a C string to a file descriptor.
 * @param file  Destination file descriptor.
 * @param s   NUL-terminated string.
 */
 void write_str(int file, const char *s) {
  write(file, s, (unsigned int)mystrlen(s));
}

/**
 * @brief Check if a char is space or tab.
 * @param c   Character.
 * @return non-zero if space or tab, else 0.
 */
 int is_space(char c) { return c==' ' || c=='\t'; }

/**
 * @brief Check if a char is one of | < > &.
 * @param c   Character.
 * @return non-zero if special, else 0.
 */
 int is_special(char c) { return c=='|' || c=='<' || c=='>' || c=='&'; }


void prompt(){ write(1, "$ ", 2); }

// Main

int main(int argc, char *argv[], char *envp[]) {
  while (1) {
    Job job = get_job();
    if (job.num_stages == 0) continue;   // empty line
    if (isExit(&job.pipeline[0])) break; // exit/Exit/EXIT
    run_job(&job, envp);
    free_all();                          // free tokens from last run
  }
  return 0;
}

// Input

/**
 * @brief Read one line from stdin into buffer (no newline kept).
 * @param input_length  Out: number of chars stored (no NUL).
 */
 void read_input(int *input_length) {
  char c; int i = 0;
  int n = read(0, &c, 1);
  while (n > 0 && c != '\n' && i < MAX_CHAR - 1) {
    buffer[i++] = c;
    n = read(0, &c, 1);
  }
  buffer[i] = '\0';
  *input_length = i;
}

/**
/**
 * @brief This function prompts the user for input, reads the input from
 *        the keyboard, creates a Job structure, and parses the input into
 *        the Job structure.
 *
 * @return Job A Job structure to store the parsed job.
 */

Job get_job() {
  slay_zombies();             // slay those pesky zombies
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

// Parsing

/**
 * @brief Skip over initial white space until a non-white space char is found.
 *
 * @param start Pointer to the starting index to modify.
 * @param input_length The total length of the input buffer.
 */
 void find_start(int *start, int *input_length) {
  while (*start < *input_length && is_space(buffer[*start])) (*start)++;
}

/**
 * @brief Find the end index of the current token by searching for the next
 *        white space character or the end of the input.
 *
 * @param start Pointer to the starting index of the token.
 * @param end Pointer to the ending index of the token to modify.
 * @param input_length The total length of the input buffer.
 */
 void find_tkn_end(int *start, int *end, int *input_length) {
  *end = *start;
  while (*end < *input_length && !is_space(buffer[*end])) (*end)++;
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
 * @brief Reap finished background children without blocking the parent.
 */
void slay_zombies() {
  int status; pid_t pid;
  while ((pid = waitpid(-1, &status, WNOHANG)) > 0) { pid; status; }
}
