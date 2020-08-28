#include <sys/types.h> // pid_t

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
extern sigset_t jobserver_sigchld(int how);
extern void jobserver_close_(struct jobserver * js, bool inherit);

// internal.c
extern int read_from_pipe(int fd, char * token);
extern int write_to_pipe(int fd, const char * buf, size_t count);
extern void close_pipe_end(int fd);

// handle.c
extern int jobserver_terminate_job(struct jobserver * js, char * token);

// token.c
extern int acquire_jobserver_token(struct jobserver * js, char * token);
extern int release_jobserver_token(struct jobserver * js, char token);

// wait.c
extern int jobserver_wait_(struct jobserver * js, int timeout, char * token);
