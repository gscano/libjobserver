#include <assert.h> // assert()
#include <errno.h> // errno
#include <signal.h> // kill()
#include <string.h> // memcpy()
#include <stdlib.h> // exit(), realloc()
#include <unistd.h> // fork()

#include "jobserver.h"
#include "internal.h"

int jobserver_launch_job(struct jobserver * js, int wait, bool shared, void * data,
			 jobserver_callback_t func, jobserver_callback_return_t done)
{
  char token;
  int status = acquire_jobserver_token_(js, wait, &token);

  switch(status)
    {
    case  0: errno = EAGAIN; return -1;
    case -1: return -1;// errno: EBADF, ECHILD, EINTR, ENOMEM
    }

  if(js->current_jobs == js->max_jobs)
    {
      const size_t max_jobs = JOBSERVER_SIZE_GEOME_PROG * js->max_jobs + JOBSERVER_SIZE_ARITH_PROG;
      js->jobs = realloc(js->jobs, max_jobs * sizeof(struct jobserver_job));
      if(js->jobs == NULL) goto error;// errno: ENOMEM
      js->max_jobs = max_jobs;
    }

  struct jobserver_job * job = &js->jobs[js->current_jobs];

  job->pid = fork();

  if(job->pid == -1)
    {
      goto error;// errno: EAGAIN, ENOMEM
    }
  else if(job->pid == 0)
    {
      jobserver_close_(js, shared);
      _exit(func(data));
    }
  else
    {
      job->token = token;
      job->done = done;
      job->data = data;

      js->current_jobs++;
    }

  return 0;

 error:;
  int error = errno;
  release_jobserver_token_(js, token);
  errno = error;
  return -1;
}

inline
struct jobserver_job * jobserver_find_job_(struct jobserver * js, pid_t pid)
{
  for(size_t i = 0; i < js->current_jobs; ++i)
    if(js->jobs[i].pid == pid)
      return &js->jobs[i];

  return NULL;
}

void jobserver_terminate_job_(struct jobserver * js, struct jobserver_job * job,
			      int status, char * token)
{
  assert(job != NULL);

  job->done(job->data, status);

  if(token == NULL)
    {
      release_jobserver_token_(js, job->token);
    }
  else
    {
      *token = job->token;
    }

  if(job != &js->jobs[js->current_jobs - 1])
    {
      memcpy(job, &js->jobs[js->current_jobs - 1], sizeof(struct jobserver_job));
    }

  js->current_jobs--;
}

int jobserver_terminate_job(struct jobserver * js, pid_t pid, int status)
{
  struct jobserver_job * job = jobserver_find_job_(js, pid);

  if(job == NULL)
    {
      errno = ECHILD;
      return -1;
    }

  jobserver_terminate_job_(js, job, status, NULL);

  return 0;
}

int jobserver_clear(struct jobserver * js)
{
  while(js->current_jobs > 0)
    {
      struct jobserver_job * job = &js->jobs[js->current_jobs - 1];

      if(kill(job->pid, SIGKILL) == -1)
	return js->current_jobs;

      if(jobserver_wait_for_job_(js, NULL, false) == -1)
	return js->current_jobs;
    }

  return 0;
}
