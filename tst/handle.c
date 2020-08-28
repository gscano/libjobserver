#include <assert.h> // assert()
#include <errno.h> // errno
#include <stdio.h> // fprintf()
#include <stdlib.h> // EXIT_SUCCESS
#include <unistd.h> // pipe(), sleep()

#include "jobserver.h"
#include "internal.h"

struct data
{
  int id;
  int sleep;
  int ret;
};

int pipefd[2];

int begin(void * data_)
{
  struct data * data = data_;

  fprintf(stderr, "launch %d\n", data->id);

  sleep(data->sleep);

  assert(write(pipefd[1], "t", 1) == 1);

  return data->ret;
}

void end(void * data_, int status)
{
  struct data * data = data_;

  fprintf(stderr, "%d terminates with %d\n", data->id, status);
}

int main()
{
  assert(pipe(pipefd) == 0);

  struct jobserver js;
  struct data data;
  char token;

  assert(jobserver_create(&js, 3, 't') != -1);

  assert(jobserver_terminate_job(&js, &token) == -1);

  data.id = 1;
  data.sleep = 1;
  data.ret = 1;
  assert(jobserver_launch_job(&js, true, &data, begin, end) == 0);
  assert(jobserver_terminate_job(&js, &token) == -1 && errno == ECHILD);
  assert(jobserver_close(&js) == -1);
  assert(read(pipefd[0], &token, 1) == 1);
  sleep(1);
  assert(jobserver_terminate_job(&js, &token) == 0);
  assert(jobserver_close(&js) == 0);

  close(pipefd[0]);
  close(pipefd[1]);

  return EXIT_SUCCESS;
}
