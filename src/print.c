#include <stdio.h> // fprintf()

#include "jobserver.h"
#include "internal.h"

int jobserver_print_job_(FILE * stream, struct jobserver_job job, const char * separator)
{
  return fprintf(stream,
		 "token: '%c'" "%s"
		 "pid: %d" "%s"
		 "exit: %p" "%s"
		 "data: %p" "%s"
		 "id: %zu",
		 job.token, separator,
		 job.pid, separator,
		 job.exit, separator,
		 job.data, separator,
		 job.id);
}

int jobserver_print_jobs_(FILE * stream, struct jobserver_job const * jobs, size_t size,
			  const char * separator, const char * inter_separator)
{
  int count = 0;

  size_t i;
  for(i = 0; i < size; ++i)
    {
      if(i != 0) count += fprintf(stream, "%s", inter_separator);
      count += jobserver_print_job_(stream, jobs[i], separator);
    }

  return count;
}

int jobserver_print(FILE * stream, struct jobserver const * js,
		    const char * separator, const char * job_separator,
		    const char * inter_job_separator)
{
  int size = fprintf(stream,
		     "dry run: %d" "%s"
		     "stopped: %d" "%s"
		     "read: %d" "%s"
		     "write: %d" "%s"
		     "size: %zu" "%s"
		     "has-free-token: %d" "%s"
		     "jobs: %zu",
		     js->dry_run, separator,
		     js->stopped, separator,
		     js->poll[1].fd, separator,
		     js->write, separator,
		     js->size, separator,
		     js->has_free_token, separator,
		     js->current_jobs);

  if(0 < js->current_jobs)
    {
      size += fprintf(stream, "%s", inter_job_separator);
      size += jobserver_print_jobs_(stream, js->jobs, js->current_jobs,
				    job_separator, inter_job_separator);
    }

  return size;
}
