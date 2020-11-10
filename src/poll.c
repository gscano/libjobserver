#include <poll.h> // poll()
#include <stdbool.h> // bool

// The pipe is open in this process, POLLERR and POLLUP would only appear
// if the current process closes the pipe manually: should it be our concern?

#if USE_SIGNALFD

int jobserver_poll_(struct pollfd pollfd[2], int timeout, bool use_pipe)
{
  pollfd[1].revents = 0;

  if(poll(pollfd, 1 + (pollfd[1].fd != -1 && use_pipe), timeout) == -1)
    return -1;// errno: EINTR, ENOMEM

  return 2 * (pollfd[0].revents & POLLIN) + (pollfd[1].revents & POLLIN);
}

#else // ! USE_SIGNALFD = self pipe trick

#include <errno.h> // errno
#include <unistd.h> // read()

int jobserver_poll_(struct pollfd pollfd[2], int timeout, bool use_pipe)
{
  pollfd[1].revents = 0;
  errno = 0;

  if(poll(pollfd, 1 + (pollfd[1].fd != -1 && use_pipe), timeout) == -1 && errno != EINTR)
    return -1;// errno: ENOMEM

  if(errno == EINTR)
    {
      char value;

      if(read(pollfd[0].fd, &value, sizeof(char)) != 1)
	{
	  errno = EINTR;
	  return -1;
	}
    }

  return 2 * (errno == EINTR || pollfd[0].revents & POLLIN) + (pollfd[1].revents & POLLIN);
}

#endif

int jobserver_has_tokens_(struct pollfd pipe)
{
  if(poll(&pipe, 1, 0) == -1)
    return -1;// errno: ENOMEM

  return pipe.revents & POLLIN;
}
