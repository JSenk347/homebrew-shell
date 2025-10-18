#ifndef RUN_JOB_H
#define RUN_JOB_H
#include <sys/types.h>
#include "jobs.h"

void open_redirections(const Job *job, int *in_fd, int *out_fd);
void close_redirections(int in_fd, int out_fd);
int set_pipe(int is_last, int pipe_fd[2]);
int wait_fg_children(pid_t pids[], int n);
void child_io(int i, int n, int in_fd, int out_fd,
                int prev_read_file, int pipe_fd[2],
                int *child_in, int *child_out);
void parent_after_fork(int child_in, int in_fd, int child_out,
                       int is_last, int *prev_read_file, int pipe_fd[2]);
void run_job(Job *job, char *envp[]);

#endif