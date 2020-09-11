#include <assert.h> // assert()
#include <errno.h> // errno
#include <stdio.h> // fprintf(), perror()
#include <unistd.h> // sleep()
#include <stdlib.h> // atoi(), EXIT_FAILURE, EXIT_SUCCESS
#include <string.h> // strncat(), strcpy(), strlen()

#include "jobserver.h"

#define MAX_ID_LENGTH 31
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
  assert(size >= 0 && size <= 26);

  char * exe = argv[2];

  const int shift = 3;

  struct jobserver js;

  char tokens[] = "abcdefghijklmnopqrstuvwxyz";
  tokens[size] = '\0';

  fprintf(stderr, "Connecting to jobserver ...");
  if(jobserver_connect(&js) == -1)
    {
      fprintf(stderr, " no jobserver found.\nCreating jobserver ...");
      assert(jobserver_create_n(&js, tokens) == size + 1);
    }
  fprintf(stderr, " done: ");
  jobserver_print(stderr, &js, ", ", ",", "\n");
  fprintf(stderr, "\n");

  char * base = getenv(LOCAL_ENV);
  if(base == NULL) base = "";
  const size_t length = strlen(base);

  if(length > MAX_ID_LENGTH - 1)
    {
      fprintf(stderr, "Id length %d too long (max %d).", (int)length, MAX_ID_LENGTH);
      return EXIT_FAILURE;
    }

  struct data jobs[argc - shift];
  char name = 'A';

  for(int i = 0; i < argc - shift; ++i, ++name)
    {
      jobs[i].exe = exe;

      strncat(jobs[i].id, base, length);
      *(jobs[i].id + length) = name;
      *(jobs[i].id + length + 1) = '\0';

      jobs[i].arg = argv[shift + i];

      fprintf(stderr, "Job %s '%s %s' prepared ...\n", jobs[i].id, jobs[i].exe, jobs[i].arg);
#if 0
      int status;
      while((status = jobserver_launch_job(&js, -1, true, &jobs[i], test, end)) != 0)
	{
	  fprintf(stderr, "Error: %d %m (stopped: %d)\n", status, js.stopped);
	  jobserver_print(stderr, &js, ", ", ",", "\n");
	  fprintf(stderr, "\n");
	  assert(errno == ECHILD);
	}
#else
      assert(jobserver_launch_job(&js, -1, true, &jobs[i], test, end) == 0);
#endif

      jobserver_print(stderr, &js, ", ", ",", "\n");
      fprintf(stderr, "\n");
    }

  int status;
  while((status = jobserver_collect(&js, -1)) != 0)
    {
      if(status == -1 && js.stopped != -1)
	{
	  fprintf(stderr, "Error: %d %m\n", status);
	  jobserver_print(stderr, &js, ", ", ",", "\n");
	  fprintf(stderr, "\n");
	  return EXIT_FAILURE;
	}
    }

  assert(jobserver_clear(&js) == 0);
  assert(jobserver_close(&js) == 0);

  return EXIT_SUCCESS;
}
