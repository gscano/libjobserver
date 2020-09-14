#include <assert.h> // assert()
#include <errno.h> // errno
#include <signal.h> // SIGCHLD
#include <unistd.h> // read()

#include "internal.h"

int jobserver_wait_(struct jobserver * js, int timeout, char * token)
{
  switch(jobserver_poll_(js->poll, timeout, token != NULL))
    {
    case -1: return -1;// errno: EINTR, ENOMEM
    default: return 0;
    case 1: return read_from_pipe_(js->read, token);
    case 2:
    case 3:
      {
	jobserver_read_sigchld_(js->poll[0].fd);

	int status = jobserver_terminate_job_(js, token, true);
	if(status != 0) return -1;// errno: ECHILD

	while(jobserver_terminate_job_(js, NULL, false) == 0) continue;

	return 1;
      }
    }
}

int jobserver_wait(struct jobserver * js, int timeout)
{
  jobserver_wait_(js, timeout, NULL);

  return js->current_jobs;
}

int jobserver_collect(struct jobserver * js, int timeout)
{
  size_t before = js->current_jobs + 1;

  while(0 < js->current_jobs && js->current_jobs < before)
    {
      before = js->current_jobs;
      if(jobserver_wait_(js, timeout, NULL) == -1)
	break;
    }

  return js->current_jobs;
}
