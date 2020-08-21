#include "jobserver.h"

#include <assert.h> // assert()
#include <errno.h> // errno
#include <fcntl.h> // fcntl()
#include <stdlib.h> // EXIT_SUCCESS
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

  setenv("MAKEFLAGS", "", 1);

  test_connect();

  setenv("MAKEFLAGS", "", 1);

  test_create();

  setenv("MAKEFLAGS", env_, 1);

  return EXIT_SUCCESS;
}


void test_connect()
{
  {
    struct jobserver js;
    assert(jobserver_connect(&js) == 0);
    assert(js.has_free_token);
  }

  {
    int pipefd[2];
    assert(pipe(pipefd) == 0);
    char env[32] = {0};

    {
      struct jobserver js;

      assert(jobserver_connect(&js) == 0);
      assert(js.has_free_token);
    }

    {
      struct jobserver js;

      snprintf(env, 32, "--jobserver-auth=%d,%d", pipefd[0], pipefd[1]);
      setenv("MAKEFLAGS", env, 1);

      assert(jobserver_connect(&js) == 0);
      assert(errno == 0);
      assert(js.has_free_token);
    }

    {
      struct jobserver js;

      snprintf(env, 32, "--jobserver-auth=%d,%d", pipefd[0], -1);
      setenv("MAKEFLAGS", env, 1);

      assert(jobserver_connect(&js) == -1);
      assert(errno == EBADF);
      assert(!js.has_free_token);
    }

    {
      struct jobserver js;

      snprintf(env, 32, "--jobserver-auth=%d,%d", -1, pipefd[1]);
      setenv("MAKEFLAGS", env, 1);

      assert(jobserver_connect(&js) == -1);
      assert(errno == EBADF);
      assert(!js.has_free_token);
    }

    assert(close(pipefd[0]) != -1);
    assert(close(pipefd[1]) != -1);
  }

  {
    struct jobserver js;

    int pipefd[2];
    assert(pipe(pipefd) == 0);
    char env[32] = {0};
    snprintf(env, 32, "--jobserver-auth=%d,%d", pipefd[0], pipefd[1]);
    setenv("MAKEFLAGS", env, 1);

    assert(close(pipefd[0]) != -1);
    assert(close(pipefd[1]) != -1);

    assert(jobserver_connect(&js) == -1);
    assert(errno == EACCES);
    assert(!js.has_free_token);
  }
}

void test_create()
{
  struct jobserver js;

  js.read = -1;
  js.write = -1;
  js.dry_run = false;
  js.debug = true;
  js.keep_going = false;

  {
    assert(jobserver_create_n(&js, "abcde") == 6);
    assert(js.read != -1);
    assert(js.write != -1);
    assert(js.has_free_token);

    char buffer[10] = {0};
    assert(read(js.read, buffer, 10) == 5);
    assert(strcmp(buffer, "abcde") == 0);

    jobserver_close(&js);
  }

  setenv("MAKEFLAGS", "", 1);

  {
    assert(jobserver_create(&js, 4, 'a') == 5);
    assert(js.read != -1);
    assert(js.write != -1);
    assert(js.has_free_token);

    char buffer[10] = {0};
    assert(read(js.read, buffer, 10) == 4);
    assert(strcmp(buffer, "aaaa") == 0);

    jobserver_close(&js);
  }
}
