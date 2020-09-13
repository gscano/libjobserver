#include <sys/types.h> // pid_t, sigset_t

#include "jobserver.h"

struct jobserver_job
{
  char token;
  pid_t pid;
  jobserver_callback_return_t done;
  void * data;
};

#define JOBSERVER_FREE_TOKEN (char)0

// init.c
extern void jobserver_close_(struct jobserver * js, bool inherit);

// internal.c
extern int read_from_pipe_(int fd, char * token);
extern int write_to_pipe_(int fd, const char * buf, size_t count);
extern void close_pipe_end_(int fd);

// handle.c
extern int jobserver_terminate_job_(struct jobserver * js, char * token, bool with_sigchld);

// signal.c
extern sigset_t jobserver_handle_sigchld_(int how);
extern int jobserver_signalfd_sigchld_(sigset_t sigchld);

// token.c
extern int acquire_jobserver_token_(struct jobserver * js, int wait, char * token);
extern int release_jobserver_token_(struct jobserver * js, char token);

// wait.c
extern int jobserver_wait_(struct jobserver * js, int timeout, char * token);
