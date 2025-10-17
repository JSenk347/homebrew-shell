#include "run_job.h"
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
#include <signal.h>
int g_is_background_job = 0;

/**
 * @brief Run the entire job: make pipes, fork stages, set redirs, wait if needed.
 * @param job   Parsed job with pipeline and flags.
 * @param envp  Environment for execve.
 */
 void run_job(Job *job, char *envp[]) {
  if (job->num_stages == 0) {
    return;
  }

  int in_fd;  // for '<'  
  int out_fd;  // for '>'

  open_redirections(job, &in_fd, &out_fd);

  pid_t pids[MAX_COMMANDS];
  int prev_read_file = -1;  // read end of the previous pipe
  int n = job->num_stages;


  for (int i = 0; i < n; i++) {
    int pipe_fd[2];
    int is_last = (i == n - 1);
    set_pipe(is_last, pipe_fd);

    // Decide child's stdin.
    int child_in, child_out;
   
    child_io(i, n, in_fd, out_fd,
                prev_read_file, pipe_fd,
                &child_in, &child_out);

    // Start the child for this stage.
    pids[i] = start_child(&job->pipeline[i], child_in, child_out, envp, job->is_background);
    parent_after_fork(child_in, in_fd, child_out, is_last, &prev_read_file, pipe_fd);
  }

 close_redirections(in_fd, out_fd);

  // If background, don't wait here.
  if (job->is_background) return;

  int soul_reaped = wait_fg_children(pids, n);
  if (soul_reaped) write(1, "\n", 1);
}

/**
* @brief Opens the in and out files specified by filepath in the
*        Job structure, assigning the fd's to the in_fd and out_fd
* @param job    The job that is being worked on
* @param in_fd  Input file descriptor
* @param out_fd Output file descriptor   
*/
void open_redirections(const Job *job, int *in_fd, int *out_fd) {
    int fd;
    *in_fd = -1;  // for '<'
    *out_fd = -1;  // for '>'
    // Open redirection files if present.
  if (job->infile_path != NULL) {
    fd = open(job->infile_path, O_RDONLY);
    if (fd < 0) {
      write_str(2, "open infile failed\n");
    } else {
      *in_fd = fd;
    }
  }
  if (job->outfile_path != NULL) {
    fd = open(job->outfile_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd < 0) {
      write_str(2, "open outfile failed\n");
    } else {
      *out_fd = fd;
    }
  }
}

/**
* @brief Closes in and out files
* @param in_fd  The input file descriptor
* @param out_fd The output file descriptor
*/
void close_redirections(int in_fd, int out_fd) {
  if (in_fd != -1) close(in_fd);
  if (out_fd != -1) close(out_fd);
}

/** 
* @brief Waits for all processes in this foreground pipeline to complete.
* @param pids   The array of process id's that have been created
*               to execute the job
* @param n      The number of process id's in pids[] (pipeline length)
* @return       An integer signaling a stage was killed by SIGINT or SIGQUIT.
*               FALSE = 0
*               TRUE = 1
*/
int wait_fg_children(pid_t pids[], int n) {
  // Foreground: wait for all children.
  int killed_by_sig = FALSE;
  for (int i = 0; i < n; i++) {
    if (pids[i] > 0) {
      int status;
      waitpid(pids[i], &status, 0);
      if (WIFSIGNALED(status)) {
        int s = WTERMSIG(status);
        if (s == SIGINT || s == SIGQUIT) killed_by_sig = TRUE;
      }
    }
  }
  return killed_by_sig;
}

/**
* @brief Sets up the pipe data structure required to pipe output from
*        stage 1 into the input of stage 2
* @param is_last    An integer signalling whether the stage is the last
*                   in the pipeline or not
* @param pipe_fd    Array of the file descriptors for the read and write
*                   ends of the pipe
* @return           An integer indicating whether the pipe creation failed
*                   or not        
*/
int set_pipe(int is_last, int pipe_fd[2]) {
    pipe_fd[0] = -1;
    pipe_fd[1] = -1;
    if (!is_last) {
      if (pipe(pipe_fd) < 0) {
        write_str(2, "pipe failed\n");
        return -1;
      }
    }
    return 0;
}

/**
* @brief Sets up the input and output file descriptors of a child process
*        to be set to the appropriate pipe end, or to standard IO.   
* @param i          The index of the current stage in the pipeline
* @param n          The total number of stages in the pipeline
* @param in_fd      The input file descriptor for the job '<' OR '-1'
* @param out_fd     The output file descriptor for the job '>' OR '-1'  
* @param prev_read_file The read end of the previous pipe in the pipeline
* @param pipe_fd 
*/
void child_io(int i, int n, int in_fd, int out_fd,
                int prev_read_file, int pipe_fd[2],
                int *child_in, int *child_out) {
    int first = (i == 0);
    int last = (i == n - 1);
    // stdin
    if(first){
        if(in_fd >= 0) {
            *child_in = in_fd;
        } else {
            *child_in = -1; 
        }
    } else {
        *child_in = prev_read_file; 
    }
    // stdout
    if(!last){
        *child_out = pipe_fd[1];
    } else {
        if(out_fd >= 0) {
            *child_out = out_fd;
        } else {
            *child_out = -1;
        }
    }
}

/**
* @brief Parent cleaning up after spawning one stage (foreground), closes the parent's copies of the child_in 
* and child_out file descriptors, if necessary it will carry pip_fd[0] forward to the next stage as prev_read_file.
* @param child_in    FD for child's stdin
* @param in_fd       '<' jobs FD 
* @param child_out   FD for child's stdout
* @param is_last     Whether this is the last stage in the pipeline
* @param prev_read_file FD for the read end of the previous pipe
* @param pipe_fd     Array containing the pipe file descriptors
*/
void parent_after_fork(int child_in, int in_fd, int child_out,
                            int is_last, int *prev_read_file, int pipe_fd[2]) {
    // Parent no longer needs these ends.                            
    if (child_in != -1 && child_in != in_fd) close(child_in);
    if (child_out != -1) close(child_out);
    // Carry the read end forward for the next stage.
    if (!is_last) {
      *prev_read_file = pipe_fd[0];
    } else {
      if (*prev_read_file != -1) {
        close(*prev_read_file);
      }
      *prev_read_file = -1;
    }                          
}

void setup_bg_child_group(int is_background) {
  if (!is_background) return;
  setpgid(0, 0); 
}

void setup_bg_process_group(const Job *job, pid_t child_pid, pid_t *pipeline_pgid) {
  if (!job || !job->is_background) return;
  if (child_pid <= 0) return;

  if (*pipeline_pgid == 0) *pipeline_pgid = child_pid;
  setpgid(child_pid, *pipeline_pgid); 
}