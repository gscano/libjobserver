#include <assert.h> // assert()
#include <errno.h> // errno
#include <signal.h> // kill()
#include <string.h> // memcpy()
#include <stdlib.h> // exit(), realloc()
#include <sys/wait.h> // waitpid()
#include <unistd.h> // fork()

#include "jobserver.h"
#include "internal.h"

int jobserver_launch_job(struct jobserver * js, bool inherit, void * data,
			 jobserver_callback_t func, jobserver_callback_return_t done)
{
  char token;
  if(acquire_jobserver_token(js, &token) == -1) return -1;

  if(js->current_jobs == js->max_jobs)
    {
      js->jobs = realloc(js->jobs, (2 * js->max_jobs + 1) * sizeof(struct jobserver_job));
      if(js->jobs == NULL) goto error;// errno: ENOMEM
      js->max_jobs = 2 * js->max_jobs + 1;
    }

  struct jobserver_job * job = &js->jobs[js->current_jobs];

  job->pid = fork();

  if(job->pid == -1)
    {
      goto error;// errno: EAGAIN, ENOMEM
    }
  else if(job->pid == 0)
    {
      jobserver_close_(js, inherit);
      exit(func(data));
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
  release_jobserver_token(js, token);
  errno = error;
  return -1;
}

static inline
struct jobserver_job * jobserver_find_job(struct jobserver * js, pid_t pid)
{
  for(size_t i = 0; i < js->current_jobs; ++i)
    if(js->jobs[i].pid == pid)
      return &js->jobs[i];

  return NULL;
}

int jobserver_terminate_job(struct jobserver * js, char * token)
{
  int status;
  pid_t pid = waitpid(-1, &status, WNOHANG);

  if(pid <= 0) return -1;// errno: ECHILD

  struct jobserver_job * job = jobserver_find_job(js, pid);

  if(job == NULL)
    {
      errno = ECHILD;
      return -1;
    }

  job->done(job->data, status);

  if(token == NULL)
    {
      if(release_jobserver_token(js, job->token) == -1) return -1;
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

  return 0;
}

int jobserver_clear(struct jobserver * js)
{
  for(size_t i = 0; 0 < js->current_jobs; ++i)
    {
      struct jobserver_job * job = &js->jobs[js->current_jobs - 1];

      if(kill(job->pid, SIGKILL) == -1) return -1;

      js->current_jobs--;
    }

  return 0;
}
