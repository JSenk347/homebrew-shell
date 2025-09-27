#ifndef JOBS_H
#define JOBS_H

#define MAX_ARGS 10     //allows 10 arguments

struct Command
{
  char *argv[MAX_ARGS+1];
  unsigned int argc;
};

typedef struct Command Command;
#endif
