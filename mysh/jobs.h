/**
 * Summary: @todo
 * 
 * @authors Anna Running Rabbit, Joseph Mills, Jordan Senko
 * @file jobs.h
 */

#ifndef JOBS_H
#define JOBS_H

#define MAX_ARGS 10     //allows 10 arguments
#define MAX_PIPELINE_LEN 10

struct Command
{
  char *argv[MAX_ARGS+1];
  unsigned int argc;
};

struct Job
{
  struct Command pipeline[MAX_PIPELINE_LEN];
  unsigned int num_stages;
  char *outfile_path; /* NULL for no output redirection     */
  char *infile_path; /* NULL for no input redirection      */
  int is_background; /* 0=foreground, otherwise=background */
};

typedef struct Job Job;
typedef struct Command Command;
#endif
