/**
 * @brief File contains all functions related to parsing the user's command line
 *
 * @authors Anna Running Rabbit, Joseph Mills, Jordan Senko
 *
 * @file commands.h
 */

#include "mystring.h"
#include "run_job.h"
#include "jobs.h"
#include "myheap.h"
#include "commands.h"
#include "mysh.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

/**
 * @brief Add a token from buffer to cmd->argv (up to MAX_ARGS).
 * @param cmd           Command to update.
 * @param token_length  Length of token.
 * @param curr          In: token start index in buffer.
 */
void add_arg(Command *cmd, int token_length, int *curr)
{
  if (token_length <= 0)
    return;
  if (cmd->argc < MAX_ARGS)
  {
    char *token = alloc((unsigned int)token_length + 1);
    for (int i = 0; i < token_length; i++)
    {
      token[i] = buffer[*curr + i];
    }
    token[token_length] = '\0';
    cmd->argv[cmd->argc++] = token;
  }
  else
  {
    write_str(2, "Too many args\n");
  }
}

/**
 * @brief Parse one command (argv) until a special symbol or end of line.
 * @param cmd           Out: argv/argc.
 * @param input_index   In/Out: scan position in buffer.
 * @param input_length  In: total buffer length.
 */
void parse_command(Command *cmd, int *input_index, int *input_length)
{
  cmd->argc = 0;

  int curr = *input_index;
  int end = *input_index;

  find_start(&curr, input_length);
  if (curr >= *input_length)
  {
    cmd->argv[0] = NULL;
    *input_index = curr;
    return;
  }

  while (curr < *input_length)
  {
    if (is_special(buffer[curr]))
    {
      *input_index = curr;
      break;
    }

    find_tkn_end(&curr, &end, input_length);
    add_arg(cmd, end - curr, &curr);

    curr = end;
    *input_index = curr;
    find_start(&curr, input_length);

    if (curr < *input_length && is_special(buffer[curr]))
    {
      *input_index = curr;
      break;
    }
  }

  cmd->argv[cmd->argc] = NULL;
  *input_index = curr;
}

/**
 * @brief Parse a pipeline: cmd [| cmd] ... and collect '<'/'>' anywhere.
 * @param job           Out: pipeline stages + infile/outfile.
 * @param input_index   In/Out: position after the parsed pipeline.
 */
void parse_pipeline(Job *job, int *input_index)
{
  int input_length = mystrlen(buffer);
  int curr = *input_index;

  Command cmd;
  parse_command(&cmd, &curr, &input_length);
  if (cmd.argc > 0)
    job->pipeline[job->num_stages++] = cmd;

  while (1)
  {
    // absorb any redirection here (job-wide)
    find_start(&curr, &input_length);
    while (curr < input_length && (buffer[curr] == '<' || buffer[curr] == '>'))
    {
      char op = buffer[curr++];
      find_start(&curr, &input_length);
      if (curr >= input_length)
      {
        write_str(2, (op == '<') ? "syntax error: missing infile after '<'\n"
                                 : "syntax error: missing outfile after '>'\n");
        *input_index = curr;
        return;
      }
      int end;
      find_tkn_end(&curr, &end, &input_length);
      if (end == curr)
      {
        write_str(2, (op == '<') ? "syntax error: empty infile\n" : "syntax error: empty outfile\n");
        *input_index = curr;
        return;
      }
      if (op == '<')
        job->infile_path = copy_token_from_buffer(curr, end);
      else
        job->outfile_path = copy_token_from_buffer(curr, end);
      curr = end;
      find_start(&curr, &input_length);
    }

    if (curr < input_length && buffer[curr] == '|')
    {
      curr++; // skip '|'
      parse_command(&cmd, &curr, &input_length);
      if (cmd.argc == 0)
      {
        write_str(2, "syntax error: empty command in pipeline\n");
        *input_index = curr;
        return;
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
void parse_job(Job *job, int *input_index)
{
  int input_length = mystrlen(buffer);
  parse_pipeline(job, input_index);

  int curr = *input_index;
  find_start(&curr, &input_length);

  if (curr < input_length)
  {
    if (buffer[curr] == '&')
    {
      job->is_background = TRUE;
      curr++;
      find_start(&curr, &input_length);
      if (curr < input_length)
        write_str(2, "warning: text after '&' ignored\n");
    }
    else
    {
      write_str(2, "syntax error: unexpected token after pipeline/redirection\n");
    }
  }

  *input_index = curr;
}

/**
 * @brief Start one pipeline stage in a child and hook up its stdin/stdout.
 * @param cmd    Stage command and argv.
 * @param file_in  file to become stdin (or -1 to keep current).
 * @param file_out file to become stdout (or -1 to keep current).
 * @param envp   Environment for execve.
 * @return Child pid in parent; child does not return on success.
 */
pid_t start_child(Command *cmd, int file_in, int file_out, char *envp[])
{
  pid_t pid = fork();
  if (pid < 0)
  {
    write_str(2, "fork failed\n");
    return -1;
  }

  if (pid == 0) // Child tings
  {
    if (file_in != -1) dup2(file_in, 0); // Hook up stdin (file 0) if requested.
    if (file_out != -1) dup2(file_out, 1); // Hook up stdout (file 1) if requested.
    if (file_in > 2) close(file_in); // These original files are no longer needed in the child.
    if (file_out > 2) close(file_out);

    cmd->argv[cmd->argc] = NULL; // exec needs argv[] to be NULL-terminated.

    signal(SIGINT, SIG_DFL); // Child should use default handlers so Ctrl-C kills it as usual.
    signal(SIGQUIT, SIG_DFL);

    // Get an absolute path (either a copy of "cmd" if it has '/', or /usr/bin/cmd).
    char *full_path = resolve_path(cmd->argv[0]);
    execve(full_path, cmd->argv, envp);

    write_str(2, "execve failed\n");
    _exit(127);
  }

  // Parent tings
  return pid;
}

/**
 * @brief Copy text [start, end) from the global buffer.
 * @param start  Inclusive index in buffer.
 * @param end    Exclusive index in buffer.
 * @return heap string (myheap) with NUL terminator.
 */
char *copy_token_from_buffer(int start, int end)
{
  int len = end - start;
  char *s = alloc((unsigned int)len + 1);
  for (int i = 0; i < len; i++) {s[i] = buffer[start + i];}
  s[len] = '\0'; // null terminating the token
  return s;
}

/**
 * @brief Turn argv[0] into an absolute path.
 *        If it has '/', use it as-is (copied). Otherwise use "/usr/bin/<cmd>".
 * @param cmd  Command name.
 * @return heap string (myheap) path.
 */
char *resolve_path(const char *cmd)
{
  for (int i = 0; cmd[i] != '\0'; i++)
  {
    if (cmd[i] == '/')
    {
      unsigned int L = mystrlen(cmd);
      char *p = alloc(L + 1);
      for (unsigned int j = 0; j < L; j++)
        p[j] = cmd[j];
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

/**
 * @brief Read one line from stdin into buffer (no newline kept).
 * @param input_length  Out: number of chars stored (no NUL).
 */
void read_input(int *input_length)
{
  char c;
  int i = 0;
  int n = read(0, &c, 1);
  while (n > 0 && c != '\n' && i < MAX_CHAR - 1)
  {
    buffer[i++] = c;
    n = read(0, &c, 1);
  }
  buffer[i] = '\0';
  *input_length = i;
}

/**
 * @brief Skip over initial white space until a non-white space char is found.
 *
 * @param start Pointer to the starting index to modify.
 * @param input_length The total length of the input buffer.
 */
void find_start(int *start, int *input_length)
{
  while (*start < *input_length && is_space(buffer[*start]))
    (*start)++;
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
  while (*end < *input_length && !is_space(buffer[*end]))
    (*end)++;
}

// BABY HELPERS
/**
 * @brief Write a C string to a file descriptor.
 * @param file  Destination file descriptor.
 * @param s   NUL-terminated string.
 */
void write_str(int file, const char *s)
{
  write(file, s, (unsigned int)mystrlen(s));
}

/**
 * @brief Check if a char is space or tab.
 * @param c   Character.
 * @return non-zero if space or tab, else 0.
 */
int is_space(char c) { return c == ' ' || c == '\t'; }

/**
 * @brief Check if a char is one of | < > &.
 * @param c   Character.
 * @return non-zero if special, else 0.
 */
int is_special(char c) { return c == '|' || c == '<' || c == '>' || c == '&'; }

/**
 * @brief Prompts the user with
 */
void prompt() { write(1, "(^-^): ", 7); }