#include <assert.h> // assert()
#include <errno.h> // errno

#include "jobserver.h"
#include "internal.h"

int jobserver_poll_on_sigchld(bool and_token, struct pollfd * fds, int timeout)
{
  int status;

  while((status = poll(fds, 1 + and_token, timeout)) == -1 && errno == EINVAL)
    continue;

  return status;
}

int jobserver_wait_(struct jobserver * js, int timeout, char * token)
{
  if(jobserver_poll_on_sigchld(token != NULL, &js->poll[0], timeout) == -1)
    return -1;// errno: ENOMEM

  if(js->poll[0].revents & POLLIN
    && jobserver_terminate_job(js, token) == 0)
    {
      while(jobserver_terminate_job(js, NULL) == 0) continue;

      return 1;
    }

  if(token != NULL
     && (js->poll[1].revents & POLLIN))
    {
      return read_from_pipe(js->read, token);
    }

  return 0;
}

int jobserver_wait(struct jobserver * js, int timeout)
{
  jobserver_wait_(js, timeout, NULL);

  return js->current_jobs;
}
