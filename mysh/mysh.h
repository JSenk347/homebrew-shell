
#ifndef MYSH_H
#define MYSH_H

#include "mystring.h"
#include "jobs.h"
#include "myheap.h"
#include "commands.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#define MAX_CHAR       300
#define MAX_ARGS       10
#define MAX_JOBS       10
#define MAX_COMMANDS   10
#define TRUE           1
#define FALSE          0

extern char buffer[MAX_CHAR];

// Prototypes
// void write_str(int file, const char *s);
//void soul_reaper();

// void prompt();
//  int  is_space(char c);
//  int  is_special(char c);
//  char *copy_token_from_buffer(int start, int end);
//  char *resolve_path(const char *cmd);

//  void read_input(int *input_length);

//  void find_start(int *start, int *input_length);
//  void find_tkn_end(int *start, int *end, int *input_length);


int  isExit(const Command *cmd);
Job  get_job();
void setup_signals();
void soul_reaper(int sig);
#endif
