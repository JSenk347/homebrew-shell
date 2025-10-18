#ifndef COMMANDS_H
#define COMMANDS_H
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#define MAX_CHAR       300
#define MAX_ARGS       10
#define MAX_JOBS       10
#define MAX_COMMANDS   10
#define TRUE           1
#define FALSE          0

void add_arg(Command *cmd, int token_length, int *curr);
void parse_command(Command *cmd, int *input_index, int *input_length);
void parse_pipeline(Job *job, int *input_index);
void parse_job(Job *job, int *input_index);
pid_t start_child(Command *cmd, int file_in, int file_out, char *envp[]);
void run_command(Command *cmd, char *envp[]);
char *copy_token_from_buffer(int start, int end);
char *resolve_path(const char *cmd);
void read_input(int *input_length);
void find_start(int *start, int *input_length);
void find_tkn_end(int *start, int *end, int *input_length);
void write_str(int file, const char *s);
int is_space(char c);
int is_special(char c);
void prompt();

#endif
