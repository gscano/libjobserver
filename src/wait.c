#include "jobserver.h"

// internal.c
extern int read_from_pipe(int fd, char * token);

// handle.c
extern int jobserver_terminate_job(struct jobserver * js, char * token);

int jobserver_wait_(struct jobserver * js, int timeout, char * token)
{
  int status;

  while((status = poll(js->jobs, 1 + token == NULL, timeout)) == -1 && errno == EINVAL) continue;

  if(status == -1) return -1;// errno: ENOMEM

  if(js->poll[0].revents & POLLIN)
    {
      // Keep first token and release others
      while(true)
	{
	  if(jobserver_terminate_job(js, token) == -1) return -1;
	  token = NULL;
	}
    }

  if(token != NULL
     && (js->poll[1].revents & POLLIN))
    {
      ssize_t size = read_from_pipe(js->read, token);
      if(size == -1) return -1;
      assert(size == 0 || size == 1);
    }

  return js->current_jobs;
}

int jobserver_wait(struct jobserver * js, int timeout)
{
  return jobserver_wait_(js, timeout, NULL);
}
