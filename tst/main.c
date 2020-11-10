#include <assert.h> // assert()
#include <errno.h> // errno
#include <stdio.h> // fprintf(), perror()
#include <unistd.h> // sleep()
#include <stdlib.h> // atoi(), EXIT_FAILURE, EXIT_SUCCESS
#include <string.h> // strncat(), strcpy(), strlen()

#include "jobserver.h"

#define MAX_ID_LENGTH 32
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

  fprintf(stdout, "Launching job %s '%s  %s'.\n",
	  data->id, data->exe, data->arg);
  fflush(stdout);

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
  int status = execve(data->exe, args, environ);

  if(status != 0)
    {
      fprintf(stderr, "Execve failed: %m\n");
      assert(false);
    }

  return 0;
}

void end(void * data_, int status)
{
  struct data * data = data_;

  fprintf(stdout, "Job %s '%s  %s' collected with status: %d\n",
	  data->id, data->exe, data->arg, WEXITSTATUS(status));
  fflush(stdout);

  assert(status == 0);
}

void connect_to(struct jobserver * js, char * arg)
{
  char * end;
  int size = strtol(arg, &end, 10);

  char * tokens;

  if(size == 0 && end == arg)
    {
      tokens = arg;
      size = strlen(tokens);
    }
  else
    {
      assert(size >= 0 && size <= 26);
      tokens = alloca(26);
      strcpy(tokens, "abcdefghijklmnopqrstuvwxyz");
      tokens[size] = '\0';
    }

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
      else if(errno == ENODEV || errno == EACCES)
	{
	  fprintf(stderr, ".\nCreating jobserver ...");
	  const int number = jobserver_create(js, tokens);
	  assert(number == size + 1);
	}
      else
	{
	  fprintf(stderr, ", error (%m).\n");
	  exit(EXIT_FAILURE);
	}
    }

  fprintf(stderr, " done: ");
  jobserver_print(stderr, js, ", ", ",", "\n");
  fprintf(stderr, "\n");
}

void prepare_jobs(struct data * jobs, size_t size, char * exe, char ** args)
{
  char * base = getenv(LOCAL_ENV);
  if(base == NULL) base = "";

  const size_t length = strlen(base);

  if(length > MAX_ID_LENGTH - 1)
    {
      fprintf(stderr, "Id length %d too long (max %d).\n", (int)length, MAX_ID_LENGTH);
      exit(EXIT_FAILURE);
    }

  char name = 'A';

  for(size_t i = 0; i < size; ++i, ++name)
    {
      jobs[i].exe = exe;

      memset(jobs[i].id, 0, MAX_ID_LENGTH);
      strncat(jobs[i].id, base, length);
      jobs[i].id[length] = name;
      jobs[i].id[length + 1] = '\0';

      jobs[i].arg = args[i];
    }
}

// [1]: number of tokens if not inherited (use '!' to make sure it is)
// [2]: target binary to call
// [.]: argument for each child
int main(int argc, char ** argv)
{
  fprintf(stderr, "MAKEFLAGS: %s\n", getenv("MAKEFLAGS"));

  const int shift = 3;

  if(argc < shift)
    {
      fprintf(stderr, "Usage: tokens exe [argument...]\n");

      return EXIT_FAILURE;
    }

  struct jobserver js;
  connect_to(&js, argv[1]);

  struct data jobs[argc - shift];
  prepare_jobs(jobs, argc - shift, argv[2], argv + shift);

  for(int i = 0; i < argc - shift; ++i)
    assert(jobserver_launch_job(&js, -1, true, &jobs[i], test, end) == 0);

 collect:;

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

  while((status = jobserver_close(&js)) != 0)
    {
      if(errno == EAGAIN || errno == EBUSY) goto collect;
      else
	{
	  assert(errno == EIDRM);
	  fprintf(stderr, "Error: missing tokens\n");
	  jobserver_print(stderr, &js, ", ", ",", "\n");
	  fprintf(stderr, "\n");
	  return EXIT_FAILURE;
	}
    }

  return EXIT_SUCCESS;
}
