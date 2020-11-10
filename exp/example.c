#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "jobserver.h"

int run(void * data_)// Not empty
{
  fprintf(stderr, "Launching job '%s'.\n", (char *)data_);

  int size = 1;

  char * data = data_;
  while((data = strchr(data, ' ')) != NULL)
    {
      ++size;
      ++data;
    }

  char ** args = alloca(size * sizeof(char *));

  args[0] = strtok(data_, " ");
  args[size] = NULL;

  size = 1;
  while((data = strtok(NULL, " ")) != NULL)
    args[size++] = data;

  int status = execv(args[0], args);

  if(status != 0) fprintf(stderr, "Execv failed: %m.\n");

  return status;
}

void end(void * data, int status)
{
  fprintf(stderr, "Job '%s' collected with status: %d.\n", (char *)data, status);
}

void connect_to(struct jobserver * js, char * tokens)
{
  fprintf(stderr, "Connecting to jobserver ...");

  if(jobserver_connect(js) == -1)
    {
      fprintf(stderr, " no jobserver found");

      if(*tokens == '!')
	{
	  if(errno == EACCES)
	    fprintf(stderr, " recursive make invocation without '+'");

	  fprintf(stderr, " and '!' was specified.\n");
	  exit(EXIT_FAILURE);
	}
      else if(errno == ENODEV)
	{
	  fprintf(stderr, ".\nCreating jobserver ...");

	  if(jobserver_create_n(js, atoi(tokens), 't') == -1)
	    exit(EXIT_FAILURE);

	  fprintf(stderr, " done.\n");
	}
      else
	{
	  fprintf(stderr, ", error (%m).\n");
	  exit(EXIT_FAILURE);
	}
    }
}

//Usage: tokens [cmds ...]
int main(int argc, char ** argv)
{
  const int shift = 2;

  if(argc < shift)
    return EXIT_FAILURE;

  struct jobserver js;
  connect_to(&js, argv[1]);

  for(int i = shift; i < argc; ++i)
    if(strlen(argv[i]) > 0)
      if(jobserver_launch_job(&js, -1, true, argv[i], run, end) == -1)
	return EXIT_FAILURE;

  int status;
  while((status = jobserver_collect(&js, -1)) != 0)
    if(status == -1 && errno != EINTR)
      return EXIT_FAILURE;

  if(jobserver_close(&js) != 0)
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
