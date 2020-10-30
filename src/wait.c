#include <assert.h> // assert()
#include <errno.h> // errno
#include <signal.h> // SIGCHLD
#include <unistd.h> // read()
#include <sys/wait.h> // waitpid()

#include "internal.h"

int jobserver_wait_for_job_(struct jobserver * js, char * token, bool with_sigchld)
{
  js->stopped = -1;

  int status;
  pid_t pid = waitpid(-1, &status, with_sigchld ? 0 : WNOHANG);

  if(pid <= 0) return -1;// errno: 0, (with_sigchld ?)EINTR

  struct jobserver_job * job = jobserver_find_job_(js, pid);

  if(job == NULL)
    {
      js->stopped = pid;
      js->status = status;
      errno = ECHILD;
      return -1;
    }

  jobserver_terminate_job_(js, job, status, token);

  return 0;
}

int jobserver_wait_(struct jobserver * js, int timeout, char * token)
{
  switch(jobserver_poll_(js->poll, timeout, token != NULL))
    {
    case -1: return -1;// errno: EINTR, ENOMEM
    default: return 0;
    case 1: return read_from_pipe_(js->read, token);// errno: EBADF
    case 2:
    case 3:
      {
	jobserver_read_sigchld_(js->poll[0].fd);

	int status = jobserver_wait_for_job_(js, token, true);
	if(status != 0) return -1;// errno: ECHILD, EINTR

	while((status = jobserver_wait_for_job_(js, NULL, false)) == 0)
	  continue;

	return js->stopped == -1 ? 1 : -1;// errno: ECHILD
      }
    }
}

int jobserver_wait(struct jobserver * js, int timeout)
{
  if(jobserver_wait_(js, timeout, NULL) == -1)
    return -1; //errno: EBADF, ECHILD, EINTR, ENOMEM

  return js->current_jobs;
}

int jobserver_collect(struct jobserver * js, int timeout)
{
  size_t before = js->current_jobs + 1;

  while(0 < js->current_jobs && js->current_jobs < before)
    {
      before = js->current_jobs;
      if(jobserver_wait_(js, timeout, NULL) == -1)
	return -1;// errno: ECHILD, EINTR, ENOMEM
    }

  return js->current_jobs;
}
