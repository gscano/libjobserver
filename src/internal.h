#include <sys/types.h> // pid_t, sigset_t

#include "jobserver.h"
#include "config.h"

struct jobserver_job
{
  char token;
  pid_t pid;

  jobserver_callback_return_t done;

  void * data;
  size_t id;
};

// internal.c
extern ssize_t read_from_pipe_(int fd, char * token);
extern ssize_t write_to_pipe_(int fd, const char * buf, size_t count);
extern void close_pipe_end_(int fd);

// handle.c
extern struct jobserver_job * jobserver_find_job_(struct jobserver * js, pid_t pid);
extern void jobserver_terminate_job_(struct jobserver * js, struct jobserver_job * job,
				     int status, char * token);

// poll.c
extern int jobserver_poll_(struct pollfd poll[2], int timeout, bool use_pipe);
extern int jobserver_has_tokens_(struct pollfd pipe);

// signal.c
extern int jobserver_handle_sigchld_(int how, int * fd);
extern void jobserver_read_sigchld_(int fd);

// token.c
extern int acquire_jobserver_token_(struct jobserver * js, int wait, char * token);
extern int release_jobserver_token_(struct jobserver * js, char token);

// wait.c
extern int jobserver_wait_(struct jobserver * js, int timeout, char * token);
extern int jobserver_wait_for_job_(struct jobserver * js, char * token, bool with_sigchld);
