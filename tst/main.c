#include <assert.h> // assert()
#include <errno.h> // errno
#include <stdio.h> // fprintf(), perror()
#include <unistd.h> // sleep()
#include <stdlib.h> // atoi(), EXIT_FAILURE, EXIT_SUCCESS
#include <string.h> // strncat(), strcpy(), strlen()

#include "jobserver.h"

#define MAX_ID_SIZE 10
#define EXE "main.sh"

struct data
{
  char id[MAX_ID_SIZE + 1];
  int wait;
};

int test(void * data_)
{
  struct data * data = data_;

  fprintf(stderr, "Launching job %s, wait for %ds.\n",
	  data->id, data->wait);

  char buffer[32];
  snprintf(buffer, 32, "%s", data->id);
  char * args[2] = {EXE, buffer};

  assert(execve("./main.sh", args, NULL) == 0);
}

void end(void * data_, int status)
{
  struct data * data = data_;

  fprintf(stderr, "Job %s collected with status: %d\n",
	  data->id, WEXITSTATUS(status));
}

// [1]: name
// [2]: number of tokens if not inherited
int main(int argc, char ** argv)
{
  if(argc < 3)
    return EXIT_FAILURE;

  char * name = argv[1];
  const size_t length = strlen(name);
  if(length > MAX_ID_SIZE)
    return EXIT_FAILURE;

  int size = atoi(argv[2]);
  assert(size != 0);

  struct jobserver js;

  fprintf(stderr, "Connecting to jobserver ... ");
  if(jobserver_connect(&js) == -1)
    {
      fprintf(stderr, " no jobserver found.\nCreating jobserver ...");
      assert(jobserver_create(&js, size, 't') == size + 1);
    }
  fprintf(stderr, " done: ");
  jobserver_print(stderr, &js, ", ", ",", "\n");
  fprintf(stderr, "\n");

  struct data jobs[argc - 3];

  char id = 'A';
  for(int i = 0; i < argc - 3; ++i, ++id)
    {
      strcpy(jobs[i].id, name);
      strncat(jobs[i].id + length, &id, 1);

      jobs[i].wait = atoi(argv[3 + i]);

      fprintf(stderr, "job %s prepared ...\n", jobs[i].id);
      assert(jobserver_launch_job(&js, -1, true, &jobs[i], test, end) == 0);
    }

  assert(jobserver_wait(&js, -1) == 0);

  assert(jobserver_close(&js) == 0);

  return EXIT_SUCCESS;
}
