#include <errno.h> // errno
#include <unistd.h> // close(), read(), write()

int write_to_pipe_(int fd, const char * buf, size_t count)
{
  int ret;

  // Atomic if count <= PIPE_BUF (see pipe(7))
  while((ret = write(fd, buf, count)) == -1 && errno == EINTR) continue;

  return ret;
}

int read_from_pipe_(int fd, char * token)
{
  int ret;

  while((ret = read(fd, token, 1)) == -1 && errno == EINTR) continue;

  return ret;
}

void close_pipe_end_(int fd)
{
  int errno_ = errno;
  int ret;

  while((ret = close(fd)) == -1 && errno == EINTR) continue;

  errno = errno_;
}
