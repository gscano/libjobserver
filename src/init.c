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

void jobserver_init(struct jobserver * js)
{
  js->size = 0;

  js->current_jobs = 0;
  js->max_jobs = 0;
  js->jobs = NULL;

  js->poll[0].fd = -1;
  js->poll[1].fd = -1;
  js->write = -1;
}

void jobserver_set(struct jobserver * js, int read, int write)
{
  js->poll[1].fd = read;
  js->write = write;
}

static inline
int jobserver_init_(struct jobserver * js, size_t size)
{
  js->stopped = -1;
  js->status = 0;

  js->size = size;
  js->has_free_token = true;

  js->current_jobs = js->max_jobs = 0;
  js->jobs = NULL;

  js->poll[0].events = js->poll[1].events = POLLIN;

  if(jobserver_handle_sigchld_(SIG_BLOCK, &js->poll[0].fd) == -1)
    return -1;// errno: EMFILE, ENFILE, (ENODEV, ENOMEM)

  return 0;
}

int jobserver_connect(struct jobserver * js)
{
  if(jobserver_getenv(js) == -1)
    return -1;// errno: EPROTO
  else
    return jobserver_reconnect(js);
}

int jobserver_reconnect(struct jobserver * js)
{
  if(js->poll[1].fd == -1 || js->write == -1)
    {
      if(js->poll[1].fd == js->write)
	errno = ENODEV;
      else
	errno = EBADF;

      return -1;
    }

  if(fcntl(js->poll[1].fd, F_GETFD) == -1) goto access_error;
  if(fcntl(js->write, F_GETFD) == -1) goto access_error;

  if(jobserver_init_(js, 0) == -1)
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

  js->poll[1].fd = -1;
  js->write = -1;

  if(size > 0)
    {
      int pipefds[2];

      if(pipe(pipefds) == -1)
	goto close;// errno: EMFILE, ENFILE

      js->poll[1].fd = pipefds[0];
      js->write = pipefds[1];

      ssize_t size_ = write_to_pipe_(js->write, tokens, size);
      assert(size_ != -1);
      assert((size_t)size_ == size);
      (void)size_;
    }

  if(jobserver_init_(js, size) == -1)
    goto close;// errno: 0, EMFILE, ENFILE

  if(size > 0 && jobserver_setenv(js) == -1)
    goto close;// errno: ENOMEM

  return size + 1;

 close:
  jobserver_close_(js, js->poll[1].fd == -1);

  return -1;
}

void jobserver_close_(struct jobserver * js, bool keep)
{
  if(!keep)
    {
      close(js->poll[1].fd);
      close(js->write);

      js->poll[1].fd = -1;
      js->write = -1;
    }

  if(js->jobs != NULL)
    free(js->jobs);

  jobserver_handle_sigchld_(SIG_UNBLOCK, &js->poll[0].fd);
}

int jobserver_close(struct jobserver * js)
{
  if(js->current_jobs > 0)
    {
      errno = EBUSY;
      return -1;
    }

  if(js->size > 0)
    {
      switch(jobserver_has_tokens_(js->poll[1]))
	{
	case -1:
	  return -1;// errno: ENOMEM
	case 0:
	  errno = EAGAIN;
	  return -1;
	}

      char tokens[js->size];
      size_t size = read(js->poll[1].fd, tokens, js->size);

      jobserver_close_(js, false);

      if(size < js->size)
	{
	  errno = EIDRM;
	  return -1;
	}
    }

  return 0;
}
