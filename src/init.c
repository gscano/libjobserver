#include "jobserver.h"

#include <assert.h> // assert()
#include <errno.h> // errno
#include <fcntl.h> // fcntl()
#include <limits.h> // PIPE_BUF
#include <stdlib.h> // free()
#include <string.h> // memset(), strlen()
#include <unistd.h> // pipe()

// internal.c
extern int write_to_pipe(int fd, const char * buf, size_t count);
extern void close_pipe_end(int fd);

static inline
void jobserver_init(struct jobserver * js)
{
  js->read = js->write = -1;

  js->has_free_token = false;

  js->current_jobs = js->max_jobs = 0;
  js->jobs = NULL;
}

int jobserver_connect(struct jobserver * js)
{
  jobserver_init(js);

  if(jobserver_getenv(js) == -1)
    return -1;// errno: EBADF

  if(js->read != -1 && js->write != -1)
    {
      if(fcntl(js->read, F_GETFD) == -1) goto access_error;
      if(fcntl(js->write, F_GETFD) == -1) goto access_error;
    }

  js->has_free_token = true;

  return 0;

 access_error:
  if(errno == EBADF) errno = EACCES;// Missing a leading '+'
  return -1;
}

static
int jobserver_create_(struct jobserver * js, char const * tokens, size_t size);

int jobserver_create(struct jobserver * js, size_t size, char token)
{
  char tokens[size];

  memset(tokens, token, size);

  return jobserver_create_(js, tokens, size);
}

int jobserver_create_n(struct jobserver * js, char const * tokens)
{
  return jobserver_create_(js, tokens, strlen(tokens));
}

int jobserver_create_(struct jobserver * js, char const * tokens, size_t size)
{
  jobserver_init(js);

  if(size > PIPE_BUF)
    {
      errno = EINVAL;
      return -1;
    }

  if(size > 0)
    {
      int pipefds[2];
      if(pipe(pipefds) == -1) return -1;// errno: EMFILE, ENFILE
      js->read = pipefds[0];
      js->write = pipefds[1];

      if(write_to_pipe(js->write, tokens, size) == -1)
	goto error_close_fds;
    }
  else
    {
      js->read = -1;
      js->write = -1;
    }

  if(jobserver_setenv(js) == -1)
    goto error_close_fds;// errno: ENOMEM

  js->has_free_token = true;

  return size + 1;

 error_close_fds:
  jobserver_close(js);

  return -1;
}

int jobserver_close(struct jobserver * js)
{
  if(js->read != -1)
    {
      close_pipe_end(js->read);
      js->read = -1;
    }

  if(js->write != -1)
    {
      close_pipe_end(js->write);
      js->write = -1;
    }

  js->has_free_token = false;

  if(js->jobs != NULL)
    {
      free(js->jobs);
      js->jobs = NULL;
    }

  return jobserver_unsetenv(js);
}
