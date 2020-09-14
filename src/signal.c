#include <assert.h> // assert()
#include <errno.h> // errno
#include <signal.h> // sigaddset(), SIGCHLD, SIG_BLOCK, SIG_UNBLOCK, sigemptyset(), sigprocmask()

#if USE_SIGNALFD

#include <sys/signalfd.h> // signalfd(), struct signalfd_siginfo
#include <unistd.h> // close()

int jobserver_handle_sigchld_(int how, int * fd)
{
  sigset_t sigchld;

  assert(sigemptyset(&sigchld) == 0);
  assert(sigaddset(&sigchld, SIGCHLD) == 0);

  if(how == SIG_BLOCK)
    {
      *fd = signalfd(-1, &sigchld, 0);

      if(*fd == -1)
	return -1; // errno: EMFILE, ENFILE, ENODEV, ENOMEM
    }

  assert(sigprocmask(how, &sigchld, NULL) == 0);

  return 0;
}

void jobserver_read_sigchld_(int fd)
{
  struct signalfd_siginfo si;

  int status = read(fd, &si, sizeof(struct signalfd_siginfo));

  assert(status == sizeof(struct signalfd_siginfo));
  assert(si.ssi_signo == SIGCHLD);

  (void)status;

  return 0;
}

#else // ! USE_SIGNALFD = self pipe trick

#include <fcntl.h> // fcntl(), O_NONBLOCK, O_CLOEXEC
#include <unistd.h> // close(), pipe(), write()

int jobserver_pipe_(int pipefd[2])
{
  if(pipe(pipefd) == -1)
    return -1;

  if(fcntl(pipefd[0], F_SETFL, O_NONBLOCK | O_CLOEXEC) == -1)
    goto close;

  if(fcntl(pipefd[1], F_SETFL, O_NONBLOCK | O_CLOEXEC) == -1)
    goto close;

  return 0;

 close:
  close(pipefd[0]);
  close(pipefd[1]);

  return -1;
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
      if(self_pipe[0] == -1
	 && jobserver_pipe_(self_pipe) == -1)
	  return -1;

      struct sigaction action;
      action.sa_handler = signal_handler;
      assert(sigemptyset(&action.sa_mask) == 0);
      assert(sigaddset(&action.sa_mask, SIGCHLD) == 0);
      action.sa_flags = SA_NOCLDSTOP;
      assert(sigaction(SIGCHLD, &action, NULL) == 0);

      *fd = self_pipe[0];
    }
  else // SIG_UNBLOCK
    {
      struct sigaction action;
      action.sa_handler = SIG_DFL;
      assert(sigemptyset(&action.sa_mask) == 0);
      action.sa_flags = 0;
      assert(sigaction(SIGCHLD, &action, NULL) == 0);

      close(self_pipe[0]);
      close(self_pipe[1]);

      self_pipe[0] = self_pipe[1] = -1;
    }

  return 0;
}

void jobserver_read_sigchld_(int fd)
{
  const size_t size = 32;
  char value[size];

  while(read(fd, &value, size * sizeof(char)) != -1) continue;
}

#endif
