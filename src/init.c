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
int jobserver_init_(struct jobserver * js, int size)
{
  // js->dry_run user controled

  js->stopped = -1;
  js->status = 0;

  js->size = size;
  js->has_free_token = true;

  js->current_jobs = 0;
  js->max_jobs = 0;
  js->jobs = NULL;

  js->poll[0].events = POLLIN;
  js->poll[1].events = POLLIN;

  // js->poll[1].fd do not set
  // js->write      do not set

  if(jobserver_handle_sigchld_(SIG_BLOCK, &js->poll[0].fd) == -1)
    return -1;// errno: 0, EMFILE, ENFILE, (ENODEV, ENOMEM)

  return 0;
}

bool jobserver_is_connected_(struct jobserver * js)
{
  return js->poll[0].fd >= 0;
}

int jobserver_connect_(struct jobserver * js)
{
  if(js->poll[1].fd == -1 || js->write == -1)
    {
      if(js->poll[1].fd == js->write)
	errno = ENODEV;
      else
	errno = EBADF;

      return -1;
    }

  if(fcntl(js->poll[1].fd, F_GETFD) == -1
     || fcntl(js->write, F_GETFD) == -1)
    {
      errno = EACCES;// Missing a leading '+'

      return -1;
    }

  return jobserver_init_(js, 0);// errno: 0, EMFILE, ENFILE
}

int jobserver_connect(struct jobserver * js)
{
  if(jobserver_getenv(js) == -1)
    return -1;// errno: EPROTO
  else
    return jobserver_connect_(js);
}

int jobserver_connect_to(struct jobserver * js, int read, int write, bool dry_run)
{
  js->dry_run = dry_run;

  js->poll[1].fd = read;
  js->write = write;

  return jobserver_connect_(js);
}

int jobserver_reconnect(struct jobserver * js)
{
  jobserver_close_(js, true);

  return jobserver_init_(js, 0);
}

static
int jobserver_create_(struct jobserver * js,
		      char const * tokens, size_t size, bool dry_run);

int jobserver_create(struct jobserver * js, char const * tokens, bool dry_run)
{
  return jobserver_create_(js, tokens, strlen(tokens), dry_run);
}

int jobserver_create_n(struct jobserver * js,
		       size_t size, char token, bool dry_run)
{
  char tokens[size];

  memset(tokens, token, size);

  return jobserver_create_(js, tokens, size, dry_run);
}

int jobserver_create_(struct jobserver * js,
		      char const * tokens, size_t size, bool dry_run)
{
  if(size > PIPE_BUF)// ensure write_to_pipe_() is atomic
    {
      errno = EINVAL;
      return -1;
    }

  js->poll[1].fd = -1;
  js->write = -1;

  js->dry_run = dry_run;

  if(size > 0)
    {
      int pipefds[2];

      if(pipe(pipefds) == -1)
	goto close;// errno: EMFILE, ENFILE

      js->poll[1].fd = pipefds[0];
      js->write = pipefds[1];

      ssize_t const written =  write_to_pipe_(js->write, tokens, size);

      if(written == -1 || (size_t)written < size)
	{
	  (void)close(js->poll[1].fd);
	  (void)close(js->write);

	  errno = EPIPE;
	  return -1;
	}
    }

  if(jobserver_init_(js, size) == -1)
    return -1;// errno: 0, EMFILE, ENFILE

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
      if(js->poll[1].fd != -1)
	(void)close(js->poll[1].fd);

      if(js->write != -1)
	(void)close(js->write);
    }

  if(js->jobs != NULL)
    free(js->jobs);

  if(js->poll[0].fd != -1)
    (void)jobserver_handle_sigchld_(SIG_UNBLOCK, &js->poll[0].fd);
}

int jobserver_close(struct jobserver * js)
{
  if(!jobserver_is_connected_(js))
    {
      errno = ENOTCONN;
      return -1;
    }

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

      if(size < js->size)
	{
	  errno = EIDRM;
	  return -1;
	}
    }

  jobserver_close_(js, false);

  return 0;
}
