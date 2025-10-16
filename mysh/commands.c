#include "mystring.h"
#include "jobs.h"
#include "myheap.h"
#include "commands.h"
#include "mysh.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * @brief Add a token from buffer to cmd->argv (up to MAX_ARGS).
 * @param cmd           Command to update.
 * @param token_length  Length of token.
 * @param curr          In: token start index in buffer.
 */
 void add_arg(Command *cmd, int token_length, int *curr) {
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
 void parse_command(Command *cmd, int *input_index, int *input_length) {
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
 void parse_pipeline(Job *job, int *input_index) {
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
 void parse_job(Job *job, int *input_index) {
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
 void run_job(Job *job, char *envp[]) {
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
 * @brief Copy text [start, end) from the global buffer.
 * @param start  Inclusive index in buffer.
 * @param end    Exclusive index in buffer.
 * @return heap string (myheap) with NUL terminator.
 */
 char *copy_token_from_buffer(int start, int end) {
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
 char *resolve_path(const char *cmd) {
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