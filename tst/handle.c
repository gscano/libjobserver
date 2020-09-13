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

  fprintf(stderr, "launch job #%d\n", data->id);

  sleep(data->sleep);

  assert(write(pipefd[1], "t", 1) == 1);

  return data->ret;
}

void end(void * data_, int status)
{
  struct data * data = data_;

  fprintf(stderr, "job #%d terminates with status %d\n", data->id, WEXITSTATUS(status));

  assert(WEXITSTATUS(status) == data->ret);
}

int main()
{
  assert(pipe(pipefd) == 0);

  struct jobserver js;
  char token = 0;

  {
    assert(jobserver_create(&js, 1, 't') == 2);
    assert(jobserver_terminate_job_(&js, &token, false) == -1);
    assert(jobserver_close(&js) == 0);
  }

  {
    struct data data = {1, 1, 1};
    assert(jobserver_create(&js, 2, 't') == 3);
    assert(jobserver_launch_job(&js, 0, true, &data, begin, end) == 0);
    assert(jobserver_terminate_job_(&js, &token, false) == -1 && errno == ECHILD);
    assert(jobserver_close(&js) == -1);
    assert(read(pipefd[0], &token, 1) == 1);
    sleep(1);
    assert(jobserver_terminate_job_(&js, &token, false) == 0);
    assert(token == JOBSERVER_FREE_TOKEN);
    assert(jobserver_close(&js) == 0);
  }

  {
    struct data data1 = {2, 1, 1};
    struct data data2 = {3, 3, 2};
    assert(jobserver_create(&js, 3, 't') == 4);
    assert(jobserver_launch_job(&js, 0, true, &data1, begin, end) == 0);
    assert(jobserver_launch_job(&js, 0, true, &data2, begin, end) == 0);
    assert(read(pipefd[0], &token, 1) == 1);
    sleep(1);
    assert(jobserver_terminate_job_(&js, &token, false) == 0);
    assert(token == JOBSERVER_FREE_TOKEN);
    sleep(3);
    assert(jobserver_terminate_job_(&js, &token, false) == 0);
    assert(token == 't');
    assert(jobserver_close(&js) == 0);
  }

  close(pipefd[0]);
  close(pipefd[1]);

  return EXIT_SUCCESS;
}
