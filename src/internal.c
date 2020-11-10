#include <errno.h> // errno
#include <unistd.h> // close(), read(), write()

ssize_t write_to_pipe_(int fd, const char * buf, size_t count)
{
  ssize_t ret;

  // Atomic if count <= PIPE_BUF (see pipe(7))
  while((ret = write(fd, buf, count)) == -1 && errno == EINTR)
    continue;

  return ret;// errno: EBADF, ENOSPC
}

ssize_t read_from_pipe_(int fd, char * token)
{
  ssize_t ret;

  while((ret = read(fd, token, 1)) == -1 && errno == EINTR)
    continue;

  return ret;//errno: EBADF
}

void close_pipe_end_(int fd)
{
  int errno_ = errno;
  int ret;

  while((ret = close(fd)) == -1 && errno == EINTR)
    continue;

  errno = errno_;
}
