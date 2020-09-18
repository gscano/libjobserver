#include <assert.h> // assert()
#include <errno.h> // errno
#include <fcntl.h> // fcntl()
#include <limits.h> // PIPE_BUF
#include <signal.h> // SIG_BLOCK, SIG_UNBLOCK
#include <stdlib.h> // free()
#include <string.h> // memset(), strlen()
#include <unistd.h> // pipe()

#include "jobserver.h"
#include "internal.h"

static inline
int jobserver_init_(struct jobserver * js)
{
  js->stopped = -1;

  js->has_free_token = true;

  js->current_jobs = js->max_jobs = 0;
  js->jobs = NULL;

  js->poll[0].events = js->poll[1].events = POLLIN;

  js->poll[1].fd = js->read;

  if(jobserver_handle_sigchld_(SIG_BLOCK, &js->poll[0].fd) == -1)
    return -1;// errno: EMFILE, ENFILE, (ENODEV, ENOMEM)

  return 0;
}

int jobserver_connect(struct jobserver * js)
{
  if(jobserver_getenv(js) == -1)
    return -1;// errno: EBADF, EPROTO

  if(js->read == -1 || js->write == -1)
    {
      errno = ENODEV;
      return -1;
    }

  if(fcntl(js->read, F_GETFD) == -1) goto access_error;
  if(fcntl(js->write, F_GETFD) == -1) goto access_error;

  if(jobserver_init_(js) == -1)
    return -1;// errno: 0, EMFILE, ENFILE

  return 0;

 access_error:
  errno = EACCES;// Missing a leading '+'

  return -1;
}

static
int jobserver_create_(struct jobserver * js, char const * tokens, size_t size);

int jobserver_create(struct jobserver * js, char const * tokens)
{
  return jobserver_create_(js, tokens, strlen(tokens));
}

int jobserver_create_n(struct jobserver * js, size_t size, char token)
{
  char tokens[size];

  memset(tokens, token, size);

  return jobserver_create_(js, tokens, size);
}

int jobserver_create_(struct jobserver * js, char const * tokens, size_t size)
{
  if(size > PIPE_BUF)
    {
      errno = EINVAL;
      return -1;
    }

  int pipefds[2];

  if(pipe(pipefds) == -1)
    return -1;// errno: EMFILE, ENFILE

  js->read = pipefds[0];
  js->write = pipefds[1];

  assert(write_to_pipe_(js->write, tokens, size) == (ssize_t)size);

  if(jobserver_init_(js) == -1)
    goto error_close;// errno: 0, EMFILE, ENFILE

  if(jobserver_setenv(js) == -1)
    goto error_close_all;// errno: ENOMEM

  return size + 1;

 error_close_all:
  jobserver_close_(js, true);

 error_close:
  close(js->read);
  close(js->write);

  return -1;
}

void jobserver_close_(struct jobserver * js, bool keep)
{
  if(!keep)
    {
      close_pipe_end_(js->read);
      close_pipe_end_(js->write);
    }

  if(js->jobs != NULL)
    free(js->jobs);

  jobserver_handle_sigchld_(SIG_UNBLOCK, &js->poll[0].fd);
}

int jobserver_close(struct jobserver * js)
{
  if(js->current_jobs > 0)
    return -1;

  jobserver_close_(js, false);

  return 0;
}
