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

#include "mystring.h"
#include "jobs.h"
#include "myheap.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
 
#define MAX_CHAR       300
#define MAX_ARGS       10
#define MAX_JOBS       10
#define MAX_COMMANDS   10
#define TRUE           1
#define FALSE          0

// Globals
char buffer[MAX_CHAR];  // input buffer

// Prototypes
static void write_str(int file, const char *s);
void slay_zombies();

void prompt();
static int  is_space(char c);
static int  is_special(char c);
static char *copy_token_from_buffer(int start, int end);
static char *resolve_path(const char *cmd);

static void read_input(int *input_length);

static void find_start(int *start, int *input_length);
static void find_tkn_end(int *start, int *end, int *input_length);

static void add_arg(Command *cmd, int token_length, int *curr);

static void parse_command(Command *cmd, int *input_index, int *input_length);
static void parse_pipeline(Job *job, int *input_index);
static void parse_job(Job *job, int *input_index);

pid_t start_child(Command *cmd, int file_in, int file_out, char *envp[]);
static void run_job(Job *job, char *envp[]);
static void run_command(Command *cmd, char *envp[]);

int  isExit(const Command *cmd);
Job  get_job();

// Small helpers (implementations)

/**
 * @brief Write a C string to a file descriptor.
 * @param file  Destination file descriptor.
 * @param s   NUL-terminated string.
 */
static void write_str(int file, const char *s) {
  write(file, s, (unsigned int)mystrlen(s));
}

/**
 * @brief Check if a char is space or tab.
 * @param c   Character.
 * @return non-zero if space or tab, else 0.
 */
static int is_space(char c) { return c==' ' || c=='\t'; }

/**
 * @brief Check if a char is one of | < > &.
 * @param c   Character.
 * @return non-zero if special, else 0.
 */
static int is_special(char c) { return c=='|' || c=='<' || c=='>' || c=='&'; }

/**
 * @brief Copy text [start, end) from the global buffer.
 * @param start  Inclusive index in buffer.
 * @param end    Exclusive index in buffer.
 * @return heap string (myheap) with NUL terminator.
 */
static char *copy_token_from_buffer(int start, int end) {
  int len = end - start;
  char *s = alloc((unsigned int)len + 1);
  for (int i = 0; i < len; i++) s[i] = buffer[start + i];
  s[len] = '\0';
  return s;
}

/**
 * @brief Turn argv[0] into an absolute path.
 *        If it has '/', use it as-is (copied). Otherwise use "/usr/bin/<cmd>".
 * @param cmd  Command name.
 * @return heap string (myheap) path.
 */
static char *resolve_path(const char *cmd) {
  for (int i = 0; cmd[i] != '\0'; i++) {
    if (cmd[i] == '/') {
      unsigned int L = mystrlen(cmd);
      char *p = alloc(L + 1);
      for (unsigned int j = 0; j < L; j++) p[j] = cmd[j];
      p[L] = '\0';
      return p;
    }
  }
  const char *base = "/usr/bin/";
  unsigned int Lb = mystrlen(base);
  unsigned int Lc = mystrlen(cmd);
  char *dst = alloc(Lb + Lc + 1);
  mystrcpy(base, dst);
  mystrcat(cmd, dst);
  return dst;
}
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
static void read_input(int *input_length) {
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
static void find_start(int *start, int *input_length) {
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
static void find_tkn_end(int *start, int *end, int *input_length) {
  *end = *start;
  while (*end < *input_length && !is_space(buffer[*end])) (*end)++;
}

/**
 * @brief Add a token from buffer to cmd->argv (up to MAX_ARGS).
 * @param cmd           Command to update.
 * @param token_length  Length of token.
 * @param curr          In: token start index in buffer.
 */
static void add_arg(Command *cmd, int token_length, int *curr) {
  if (token_length <= 0) return;
  if (cmd->argc < MAX_ARGS) {
    char *token = alloc((unsigned int)token_length + 1);
    for (int i = 0; i < token_length; i++) 
    {
        token[i] = buffer[*curr + i];
    }
    token[token_length] = '\0';
    cmd->argv[cmd->argc++] = token;
  } 
  else {
    write_str(2, "Too many args\n");
  }
}

/**
 * @brief Parse one command (argv) until a special symbol or end of line.
 * @param cmd           Out: argv/argc.
 * @param input_index   In/Out: scan position in buffer.
 * @param input_length  In: total buffer length.
 */
static void parse_command(Command *cmd, int *input_index, int *input_length) {
  cmd->argc = 0;

  int curr = *input_index;
  int end  = *input_index;

  find_start(&curr, input_length);
  if (curr >= *input_length) { cmd->argv[0] = NULL; *input_index = curr; return; }

  while (curr < *input_length) {
    if (is_special(buffer[curr])) { *input_index = curr; break; }

    find_tkn_end(&curr, &end, input_length);
    add_arg(cmd, end - curr, &curr);

    curr = end;
    *input_index = curr;
    find_start(&curr, input_length);

    if (curr < *input_length && is_special(buffer[curr])) { *input_index = curr; break; }
  }

  cmd->argv[cmd->argc] = NULL;
  *input_index = curr;
}

/**
 * @brief Parse a pipeline: cmd [| cmd] ... and collect '<'/'>' anywhere.
 * @param job           Out: pipeline stages + infile/outfile.
 * @param input_index   In/Out: position after the parsed pipeline.
 */
static void parse_pipeline(Job *job, int *input_index) {
  int input_length = mystrlen(buffer);
  int curr = *input_index;

  Command cmd;
  parse_command(&cmd, &curr, &input_length);
  if (cmd.argc > 0) job->pipeline[job->num_stages++] = cmd;

  while(1) {
    // absorb any redirection here (job-wide)
    find_start(&curr, &input_length);
    while (curr < input_length && (buffer[curr] == '<' || buffer[curr] == '>')) {
      char op = buffer[curr++];
      find_start(&curr, &input_length);
      if (curr >= input_length) {
        write_str(2, (op=='<') ? "syntax error: missing infile after '<'\n"
                               : "syntax error: missing outfile after '>'\n");
        *input_index = curr; return;
      }
      int end; find_tkn_end(&curr, &end, &input_length);
      if (end == curr) {
        write_str(2, (op=='<') ? "syntax error: empty infile\n" : "syntax error: empty outfile\n");
        *input_index = curr; return;
      }
      if (op == '<') job->infile_path  = copy_token_from_buffer(curr, end);
      else           job->outfile_path = copy_token_from_buffer(curr, end);
      curr = end;
      find_start(&curr, &input_length);
    }

    // next stage?
    if (curr < input_length && buffer[curr] == '|') {
      curr++; // skip '|'
      parse_command(&cmd, &curr, &input_length);
      if (cmd.argc == 0) {
        write_str(2, "syntax error: empty command in pipeline\n");
        *input_index = curr; return;
      }
      job->pipeline[job->num_stages++] = cmd;
      continue;
    }
    break;
  }

  *input_index = curr;
}

/**
 * @brief Parse whole job: pipeline (+ redirs) then optional '&'.
 * @param job           Out: full job.
 * @param input_index   In/Out: position after job.
 */
static void parse_job(Job *job, int *input_index) {
  int input_length = mystrlen(buffer);
  parse_pipeline(job, input_index);

  int curr = *input_index;
  find_start(&curr, &input_length);

  if (curr < input_length) {
    if (buffer[curr] == '&') {
      job->is_background = TRUE;
      curr++;
      find_start(&curr, &input_length);
      if (curr < input_length) write_str(2, "warning: text after '&' ignored\n");
    } else {
      write_str(2, "syntax error: unexpected token after pipeline/redirection\n");
    }
  }

  *input_index = curr;
}

// Execution

/**
 * @brief Start one pipeline stage in a child and hook up its stdin/stdout.
 * @param cmd    Stage command and argv.
 * @param file_in  file to become stdin (or -1 to keep current).
 * @param file_out file to become stdout (or -1 to keep current).
 * @param envp   Environment for execve.
 * @return Child pid in parent; child does not return on success.
 */
pid_t start_child(Command *cmd, int file_in, int file_out, char *envp[]) {
  pid_t pid = fork();
  if (pid < 0) {
    write_str(2, "fork failed\n");
    return -1;
  }

  if (pid == 0) {  // child
    // Hook up stdin (file 0) if requested.
    if (file_in != -1) {
      dup2(file_in, 0);
    }

    // Hook up stdout (file 1) if requested.
    if (file_out != -1) {
      dup2(file_out, 1);
    }

    // These original files are no longer needed in the child.
    if (file_in > 2)  close(file_in);
    if (file_out > 2) close(file_out);

    // exec needs argv[] to be NULL-terminated.
    cmd->argv[cmd->argc] = NULL;

    // Get an absolute path (either a copy of "cmd" if it has '/', or /usr/bin/cmd).
    char *full_path = resolve_path(cmd->argv[0]);
    execve(full_path, cmd->argv, envp);
    
    write_str(2, "execve failed\n");
    _exit(127);
  }

  // parent
  return pid;
}

/**
 * @brief Run the entire job: make pipes, fork stages, set redirs, wait if needed.
 * @param job   Parsed job with pipeline and flags.
 * @param envp  Environment for execve.
 */
static void run_job(Job *job, char *envp[]) {
  if (job->num_stages == 0) {
    return;
  }

  int in_file  = -1;  // for '<'
  int out_file = -1;  // for '>'

  // Open redirection files if present.
  if (job->infile_path != NULL) {
    in_file = open(job->infile_path, O_RDONLY);
    if (in_file < 0) {
      write_str(2, "open infile failed\n");
    }
  }
  if (job->outfile_path != NULL) {
    out_file = open(job->outfile_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_file < 0) {
      write_str(2, "open outfile failed\n");
    }
  }

  pid_t pids[MAX_COMMANDS];
  int prev_read_file = -1;  // read end of the previous pipe
  int n = job->num_stages;

  for (int i = 0; i < n; i++) {
    int pipefile[2];
    pipefile[0] = -1;
    pipefile[1] = -1;

    int is_last = 0;
    if (i == n - 1) {
      is_last = 1;
    }

    // If there is a next stage, create a pipe to it.
    if (!is_last) {
      if (pipe(pipefile) < 0) {
        write_str(2, "pipe failed\n");
        pipefile[0] = -1;
        pipefile[1] = -1;
      }
    }

    // Decide child's stdin.
    int child_in;
    if (i == 0) {
      if (in_file >= 0) {
        child_in = in_file;     // first stage reads from file if provided
      } else {
        child_in = -1;        // otherwise, inherit existing stdin
      }
    } else {
      child_in = prev_read_file; // later stages read from previous pipe
    }

    // Decide child's stdout.
    int child_out;
    if (!is_last) {
      child_out = pipefile[1];   // write into the new pipe
    } else {
      if (out_file >= 0) {
        child_out = out_file;    // last stage writes to file if provided
      } else {
        child_out = -1;        // otherwise, inherit existing stdout
      }
    }

    // Start the child for this stage.
    pids[i] = start_child(&job->pipeline[i], child_in, child_out, envp);

    // Parent no longer needs these ends.
    if (child_in != -1 && child_in != in_file) {
      close(child_in);
    }
    if (child_out != -1) {
      close(child_out);
    }

    // Carry the read end forward for the next stage.
    if (!is_last) {
      prev_read_file = pipefile[0];
    } else {
      if (prev_read_file != -1) {
        close(prev_read_file);
      }
      prev_read_file = -1;
    }
  }

  // Parent: close redirection files (children already have what they need).
  if (in_file  != -1) close(in_file);
  if (out_file != -1) close(out_file);

  // If background, don't wait here.
  if (job->is_background) {
    return;
  }

  // Foreground: wait for all children.
  for (int i = 0; i < n; i++) {
    if (pids[i] > 0) {
      int status;
      waitpid(pids[i], &status, 0);
    }
  }
}

/**
 * @brief Run a single command (no pipes/redirs). Handy for quick tests.
 * @param cmd  Parsed command structure (argv/argc).
 * @param envp Environment passed down from main.
 
static void run_command(Command *cmd, char *envp[]) {
  if (cmd->argc == 0) return;

  pid_t pid = fork();
  if (pid < 0) { write_str(2, "fork failed\n"); _exit(1); }

  if (pid == 0) {
    cmd->argv[cmd->argc] = NULL;
    char *full_path = resolve_path(cmd->argv[0]);
    execve(full_path, cmd->argv, envp);
    write_str(2, "execve failed\n");
    _exit(127);
  }

  int status;
  waitpid(pid, &status, 0);
}
 */
// Builtins / Debug / Reaping

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
