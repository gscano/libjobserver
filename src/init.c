#include <assert.h> // assert()
#include <errno.h> // errno
#include <fcntl.h> // fcntl()
#include <limits.h> // PIPE_BUF
#include <signal.h> // sigaddset(), SIGCHLD, sigemptyset(), sigprocmask()
#include <stdlib.h> // free()
#include <string.h> // memset(), strlen()
#include <sys/signalfd.h> // signalfd()
#include <unistd.h> // pipe()

#include "jobserver.h"
#include "internal.h"

sigset_t jobserver_sigchld_(int how)
{
  sigset_t sigchld;

  assert(sigemptyset(&sigchld) == 0);
  assert(sigaddset(&sigchld, SIGCHLD) == 0);

  assert(sigprocmask(how, &sigchld, NULL) == 0);

  return sigchld;
}

static inline
int jobserver_init_(struct jobserver * js)
{
  js->stopped = -1;

  js->has_free_token = true;

  js->current_jobs = js->max_jobs = 0;
  js->jobs = NULL;

  js->poll[0].events = js->poll[1].events = POLLIN;

  sigset_t sigchld = jobserver_sigchld_(SIG_BLOCK);

  js->poll[0].fd = signalfd(-1, &sigchld, 0);
  js->poll[1].fd = js->read;

  if(js->poll[0].fd == -1)
    goto unblock_sigchld;// errno: EMFILE, ENFILE, ENODEV, ENOMEM

  if(fcntl(js->poll[0].fd, F_SETFD, O_CLOEXEC) == -1)
    goto close_signalfd;

  return 0;

 close_signalfd:
  close(js->poll[0].fd);

 unblock_sigchld:
  jobserver_sigchld_(SIG_UNBLOCK);

  return -1;
}

int jobserver_connect(struct jobserver * js)
{
  if(jobserver_getenv(js) == -1)
    return -1;// errno: EBADF

  if(js->read == -1 || js->write == -1)
    return -1;

  if(fcntl(js->read, F_GETFD) == -1) goto access_error;
  if(fcntl(js->write, F_GETFD) == -1) goto access_error;

  if(jobserver_init_(js) == -1)
    return -1;// errno: EMFILE, ENFILE, ENODEV, ENOMEM

  return 0;

 access_error:
  if(errno == EBADF)
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

  if(write_to_pipe_(js->write, tokens, size) == -1)
    goto error_close;

  if(jobserver_init_(js) == -1)
    goto error_close;// errno: EMFILE, ENFILE, ENODEV, ENOMEM

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

  jobserver_sigchld_(SIG_UNBLOCK);
  close(js->poll[0].fd);
}

int jobserver_close(struct jobserver * js)
{
  if(js->current_jobs > 0)
    return -1;

  jobserver_close_(js, false);

  return 0;
}
