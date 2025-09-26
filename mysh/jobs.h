#ifndef JOBS_H
#define JOBS_H

#define MAX_ARGS 0     /* TO DO */

struct Command
{
  char *argv[MAX_ARGS+1];
  unsigned int argc;
};

#endif
