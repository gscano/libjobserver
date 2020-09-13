#include <assert.h> // assert()
#include <sys/signalfd.h> // signalfd()
#include <signal.h> // sigaddset(), SIGCHLD, sigemptyset(), sigprocmask()
#include <stddef.h>

sigset_t jobserver_handle_sigchld_(int how)
{
  sigset_t sigchld;

  assert(sigemptyset(&sigchld) == 0);
  assert(sigaddset(&sigchld, SIGCHLD) == 0);

  assert(sigprocmask(how, &sigchld, NULL) == 0);

  return sigchld;
}

int jobserver_signalfd_sigchld_(sigset_t sigchld)
{
  return signalfd(-1, &sigchld, 0);
}
