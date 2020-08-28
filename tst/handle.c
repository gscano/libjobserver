#include <assert.h> // assert()
#include <stdio.h> // fprintf()
#include <stdlib.h> // EXIT_SUCCESS
#include <unistd.h> // sleep()

#include "jobserver.h"
#include "internal.h"

struct data
{
  int id;
  int ret;
};

int begin(void * data_)
{
  struct data * data = data_;

  fprintf(stderr, "launch %d\n", data->id);

  return data->ret;
}

void end(void * data_, int status)
{
  struct data * data = data_;

  fprintf(stderr, "%d terminate %d\n", data->id, status);
}

int main()
{
  struct jobserver js;
  int value;
  char token;

  assert(jobserver_create(&js, 3, 't') != -1);

  value = 2;
  assert(jobserver_launch_job(&js, true, &value, begin, end) != -1);
  assert(jobserver_terminate_job(&js, &token) != -1);

  assert(jobserver_close(&js) != -1);

  return EXIT_SUCCESS;
}
