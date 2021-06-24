#include "jobserver.h"
#include "config.h"

#include <assert.h> // assert()
#include <errno.h> // errno
#include <fcntl.h> // fcntl()
#include <stdlib.h> // EXIT_SUCCESS, setenv(), unsetenv()
#include <stdio.h> // sprintf()
#include <unistd.h> // pipe()

#include <string.h> //fprintf(stderr, "%s\n", strerror(errno));

void test_connect();
void test_create();

int main()
{
  char const * env = getenv("MAKEFLAGS");
  if(env == NULL) env = "";
  char env_[strlen(env) + 1];
  strcpy(env_, env);

  unsetenv("MAKEFLAGS");

  test_connect();

  unsetenv("MAKEFLAGS");

  test_create();

  setenv("MAKEFLAGS", env_, 1);

  return EXIT_SUCCESS;
}


void test_connect()
{
  {
    struct jobserver js;
    assert(jobserver_connect(&js) == -1);
    assert(errno == ENODEV);
  }

  {
    int pipefd[2];
    assert(pipe(pipefd) == 0);
    char env[32] = {0};

    {
      struct jobserver js;

      snprintf(env, 32, ""MAKEFLAGS_JOBSERVER"=%d,%d", pipefd[0], -1);
      setenv("MAKEFLAGS", env, 1);

      assert(jobserver_connect(&js) == -1);
      assert(errno == EBADF);
    }

    {
      struct jobserver js;

      snprintf(env, 32, ""MAKEFLAGS_JOBSERVER"=%d,%d", -1, pipefd[1]);
      setenv("MAKEFLAGS", env, 1);

      assert(jobserver_connect(&js) == -1);
      assert(errno == EBADF);
    }

    {
      struct jobserver js;

      snprintf(env, 32, ""MAKEFLAGS_JOBSERVER"=%d,%d", pipefd[0], pipefd[1]);
      setenv("MAKEFLAGS", env, 1);

      assert(jobserver_connect(&js) == 0);
      assert(jobserver_close(&js) == 0);
    }

    {
      struct jobserver js;

      snprintf(env, 32, ""MAKEFLAGS_JOBSERVER"=%d,%d", pipefd[0], pipefd[1]);
      setenv("MAKEFLAGS", env, 1);

      assert(close(pipefd[0]) == -1);
      assert(close(pipefd[1]) == -1);

      assert(jobserver_connect(&js) == -1);
      assert(errno == EACCES);
    }
  }

  {
    struct jobserver js;

    int pipefd[2];
    assert(pipe(pipefd) == 0);
    char env[32] = {0};
    snprintf(env, 32, ""MAKEFLAGS_JOBSERVER"=%d,%d", pipefd[0], pipefd[1]);
    setenv("MAKEFLAGS", env, 1);

    assert(close(pipefd[0]) != -1);
    assert(close(pipefd[1]) != -1);

    assert(jobserver_connect(&js) == -1);
    assert(errno == EACCES);
  }
}

void test_create()
{
  struct jobserver js;

  js.dry_run = false;

  {
    assert(jobserver_create(&js, "") == 1);
    assert(jobserver_close(&js) == 0);
  }

  {
    assert(jobserver_create(&js, "abcde") == 6);

    char buffer[10] = {0};
    assert(read(js.poll[1].fd, buffer, 10) == 5);
    assert(strcmp(buffer, "abcde") == 0);
    assert(write(js.write, buffer, 5) == 5);

    assert(jobserver_close(&js) == 0);
  }

  setenv("MAKEFLAGS", "", 1);

  {
    assert(jobserver_create_n(&js, 4, 'a') == 5);

    char buffer[10] = {0};
    assert(read(js.poll[1].fd, buffer, 10) == 4);
    assert(strcmp(buffer, "aaaa") == 0);
    assert(write(js.write, buffer, 4) == 4);

    assert(jobserver_close(&js) == 0);
  }
}
