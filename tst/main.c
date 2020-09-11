#include <assert.h> // assert()
#include <errno.h> // errno
#include <stdio.h> // fprintf(), perror()
#include <unistd.h> // sleep()
#include <stdlib.h> // atoi(), EXIT_FAILURE, EXIT_SUCCESS
#include <string.h> // strncat(), strcpy(), strlen()

#include "jobserver.h"

#define MAX_ID_LENGTH 10
#define LOCAL_ENV "JOBSERVER_TEST"

struct data
{
  char * exe;
  char id[MAX_ID_LENGTH + 1];
  char * arg;
};

int test(void * data_)
{
  struct data * data = data_;

  fprintf(stderr, "Launching job %s '%s %s'.\n",
	  data->id, data->exe, data->arg);

  int size = 2;
  char * pos = data->arg;
  while((pos = strchr(pos, ' ')) != NULL)
    {
      ++pos;
      ++size;
    }

  char * args[size];
  args[0] = data->exe;
  args[1] = data->arg;
  size = 2;

  pos = data->arg;
  while((pos = strchr(pos, ' ')) != NULL)
    {
      *pos = '\0';
      args[size++] = ++pos;
    }

  args[size] = NULL;

  assert(setenv(LOCAL_ENV, data->id, 1) == 0);

  extern char ** environ;
  assert(execve(data->exe, args, environ) == 0);

  return 0;
}

void end(void * data_, int status)
{
  struct data * data = data_;

  fprintf(stderr, "Job %s '%s %s' collected with status: %d\n",
	  data->id, data->exe, data->arg, WEXITSTATUS(status));
}

// [1]: target binary to call
// [2]: number of tokens if not inherited
// [.]: argument for each child
int main(int argc, char ** argv)
{
  if(argc < 3)
    {
      fprintf(stderr, "Usage: target tokens [argument...]\n");
      return EXIT_FAILURE;
    }

  int size = atoi(argv[1]);
  assert(size >= 0);

  char * exe = argv[2];

  struct jobserver js;

  fprintf(stderr, "Connecting to jobserver ...");
  if(jobserver_connect(&js) == -1)
    {
      fprintf(stderr, " no jobserver found.\nCreating jobserver ...");
      assert(jobserver_create(&js, size, 't') == size + 1);
    }
  fprintf(stderr, " done: ");
  jobserver_print(stderr, &js, ", ", ",", "\n");
  fprintf(stderr, "\n");

  char * base = getenv(LOCAL_ENV);
  if(base == NULL)
    base = "A";

  const size_t length = strlen(base);

  if(length > MAX_ID_LENGTH)
    {
      fprintf(stderr, "Id length %d too long (max %d).", (int)length, MAX_ID_LENGTH);
      return EXIT_FAILURE;
    }

  const int shift = 3;
  struct data jobs[argc - shift];
  char name = 'A';

  int status = EXIT_SUCCESS;
  for(int i = 0; i < argc - shift; ++i, ++name)
    {
      jobs[i].exe = exe;

      strncat(jobs[i].id + length, base, 1);
      strcpy(jobs[i].id, &name);

      jobs[i].arg = argv[shift + i];

      fprintf(stderr, "Job %s '%s %s' prepared ...\n", jobs[i].id, jobs[i].exe, jobs[i].arg);
      int local = jobserver_launch_job(&js, -1, true, &jobs[i], test, end);

      if(local != 0)
	{
	  fprintf(stderr, "Error: %d, %m\n", local);
	  return EXIT_FAILURE;
	}
    }

  if(jobserver_collect(&js, -1) != 0)
    {
      fprintf(stderr, "Error: %m\n");
      return EXIT_FAILURE;
    }

  assert(jobserver_clear(&js));
  assert(jobserver_close(&js) == 0);

  return status;
}
