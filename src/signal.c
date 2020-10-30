#include <assert.h> // assert()
#include <errno.h> // errno
#include <signal.h> // sigaddset(), SIGCHLD, SIG_BLOCK, SIG_UNBLOCK, sigemptyset(), sigprocmask()

#define _(X) {int status = (X); assert(status == 0); (void)status;}

#if USE_SIGNALFD

#include <sys/signalfd.h> // signalfd(), struct signalfd_siginfo
#include <unistd.h> // close()

int jobserver_handle_sigchld_(int how, int * fd)
{
  sigset_t sigchld;

  _(sigemptyset(&sigchld));
  _(sigaddset(&sigchld, SIGCHLD));

  if(how == SIG_BLOCK)
    {
      *fd = signalfd(-1, &sigchld, 0);

      if(*fd == -1)
	{
	  if(errno == ENODEV || errno == ENOMEM)
	      errno = 0;

	  return -1;// errno: 0, EMFILE, ENFILE
	}
    }
  else // SIG_UNBLOCK
    {
      close(*fd);
    }

  _(sigprocmask(how, &sigchld, NULL));

  return 0;
}

void jobserver_read_sigchld_(int fd)
{
  struct signalfd_siginfo si;

  int status = read(fd, &si, sizeof(struct signalfd_siginfo));

  assert(status == sizeof(struct signalfd_siginfo));
  assert(si.ssi_signo == SIGCHLD);

  (void)status;
}

#else // ! USE_SIGNALFD = self pipe trick

#include <fcntl.h> // fcntl(), O_NONBLOCK, O_CLOEXEC
#include <unistd.h> // close(), pipe(), write()

void fcntl_set_nonblock(int fd)
{
  // Local to this process: EACCES and EAGAIN not possible
  while(fcntl(fd, F_SETFL, O_NONBLOCK) == -1
	&& errno == EINTR)
    continue;
}

int self_pipe[2] = {-1, -1};

void signal_handler(int signal)
{
  (void)signal;

  int error = errno;

  if(write(self_pipe[1], ".", sizeof(char)) == -1)
    {
      assert("Pipe full");
      errno = error;
    }
}

int jobserver_handle_sigchld_(int how, int * fd)
{
  if(how == SIG_BLOCK)
    {
      if(self_pipe[0] == -1)
	{
	  if(pipe(self_pipe) == -1)
	    return -1;// errno: EMFILE, ENFILE

	  fcntl_set_nonblock(self_pipe[0]);
	  fcntl_set_nonblock(self_pipe[1]);
	}

      struct sigaction action;
      action.sa_handler = signal_handler;
      _(sigemptyset(&action.sa_mask));
      _(sigaddset(&action.sa_mask, SIGCHLD));
      action.sa_flags = SA_NOCLDSTOP;
      _(sigaction(SIGCHLD, &action, NULL));

      *fd = self_pipe[0];
    }
  else // SIG_UNBLOCK
    {
      struct sigaction action;
      action.sa_handler = SIG_DFL;
      _(sigemptyset(&action.sa_mask));
      action.sa_flags = 0;
      _(sigaction(SIGCHLD, &action, NULL));

      close(self_pipe[0]);
      close(self_pipe[1]);

      self_pipe[0] = self_pipe[1] = -1;
    }

  return 0;
}

void jobserver_read_sigchld_(int fd)
{
  const size_t size = 32;//Should we empty the pipe at once with PIPE_BUF?
  char value[size];

  while(read(fd, &value, size * sizeof(char)) != -1)
    continue;
}

#endif
