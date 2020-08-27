#include "jobserver.h"

#include <assert.h> // assert()
#include <errno.h> // errno
#include <string.h> // memcpy()
#include <stdlib.h> // exit(), realloc()
#include <sys/wait.h> // waitpid()
#include <unistd.h> // fork()

// token.c
extern int acquire_jobserver_token(struct jobserver * js, char * token);
extern int release_jobserver_token(struct jobserver * js, char token);

// init.c
extern int jobserver_close(struct jobserver * js);

struct jobserver_job
{
  char token;
  pid_t pid;
  jobserver_callback_return_t done;
  void * data;
};

int jobserver_launch_job(struct jobserver * js, void * data,
			 jobserver_callback_t func, jobserver_callback_return_t done)
{
  char token;
  if(acquire_jobserver_token(js, &token) == -1) return -1;

  if(js->current_jobs == js->max_jobs)
    {
      js->jobs = realloc(js->jobs, (2 * js->max_jobs + 1) * sizeof(struct jobserver_job));
      if(js->jobs == NULL) goto error;
      js->max_jobs = 2 * js->max_jobs + 1;
    }

  struct jobserver_job * job = &js->jobs[js->current_jobs];

  job->pid = fork();// errno: EAGAIN, ENOMEM

  if(job->pid == -1)
    {
      goto error;
    }
  else if(job->pid == 0)
    {
      exit(func(data, js->dry_run, js->debug, js->keep_going));
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

int jobserver_terminate_job(struct jobserver * js, char * token)
{
  int status;
  int pid = waitpid(-1, &status, WNOHANG);

  assert(pid != 0);
  if(pid == -1)
    {
      assert(errno != ECHILD);
      return -1;
    }

  struct jobserver_job * job = NULL;

  for(size_t i = 0; i < js->current_jobs; ++i)
    {
      if(js->jobs[i].pid == pid)
	{
	  job = &js->jobs[i];
	  break;
	}
    }

  assert(job != NULL);

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
