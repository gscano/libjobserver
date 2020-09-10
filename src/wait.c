#include <assert.h> // assert()
#include <errno.h> // errno
#include <signal.h> // SIGCHLD
#include <sys/signalfd.h> // struct signalfd_siginfo
#include <unistd.h> // read()

#include "jobserver.h"
#include "internal.h"

int jobserver_wait_(struct jobserver * js, int timeout, char * token)
{
  if(poll(js->poll, 1 + (token != NULL), timeout) == -1)
    return -1;// errno: EINTR, ENOMEM

  if(js->poll[0].revents & POLLIN)
    {
      struct signalfd_siginfo si;

      int status = read(js->poll[0].fd, &si, sizeof(struct signalfd_siginfo));

      assert(status == sizeof(struct signalfd_siginfo));
      assert(si.ssi_signo == SIGCHLD);

      status = jobserver_terminate_job(js, token);

      assert(status == 0);
      (void)status;

      while(jobserver_terminate_job(js, NULL) == 0) continue;

      return 1;
    }

  errno = 0;

  if(token != NULL
     && js->poll[1].revents & POLLIN)
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
