#include <assert.h> // assert()
#include <stdbool.h> // bool
#include <errno.h> // errno
#include <stdio.h> // snprintf()
#include <stdlib.h> // getenv(), setenv(), strtol()
#include <string.h> // strchr(), strstr()

#define MAKEFLAGS "MAKEFLAGS"

#define MAKEFLAGS_AUTH "--jobserver-auth"

static inline
bool search_for_n_in_first_word(char const * env)
{
  bool found = false;

  while(*env != '\0' && *env != ' ' && !found)
    found = *env++ == 'n';

  return found;
}

int jobserver_getenv(int * read_fd, int * write_fd, bool * dry_run)
{
  *read_fd = -1;
  *write_fd = -1;
  *dry_run = false;

  char const * env = getenv(MAKEFLAGS);
  if(env == NULL) return 0;

  if(env[0] != '-') *dry_run = search_for_n_in_first_word(env);

  char const * fds = strstr(env, MAKEFLAGS_AUTH);
  if(fds == NULL) return 0;
  fds = strchr(fds, '=');
  if(fds == NULL) return -1;
  ++fds;

  long int fd;
  errno = 0;

  env = fds;
  fd = strtol(fds, (char **)&fds, 10);
  if(fds == env || errno) goto error;
  *read_fd = fd;

  if(*fds++ != ',') return -1;

  env = fds;
  fd = strtol(fds, (char **)&fds, 10);
  if(fds == env || errno) goto error;
  *write_fd = fd;

  return 0;

 error:
  errno = EBADF;// EINVAL or ERANGE from strtol()
  return -1;
}

int jobserver_setenv(int read_fd, int write_fd, bool dry_run)
{
  char const * env = getenv(MAKEFLAGS);
  char const * n = "";
  char const * split;
  char const * end;
  bool space = false;

  if(env == NULL)
    {
      env = "";
      if(dry_run) n = "n ";
      split = end = env;
    }
  else
    {
      end = env + strlen(env);
      if(end != env || dry_run) space = true;

      if(dry_run && !search_for_n_in_first_word(env))
	n = "n";

      split = strstr(env, "-- ");
      if(split == NULL)
	split = end;
    }

  bool space2 = split - 1 >= env && split[-1] == ' ';

#define JOBSERVER_PRINT_ENV(ptr, size)				\
  snprintf(ptr, size, "%s%.*s%s"MAKEFLAGS_AUTH"=%d,%d%s%s",	\
	   n, (int)(split - env - space2), env,			\
	   space ? " " : "",					\
	   read_fd, write_fd, split != end ? " " : "", split)

  const int size = JOBSERVER_PRINT_ENV(NULL, 0);

  if(size == -1) return -1;

  char buffer[size + 1];

  const int ssize = JOBSERVER_PRINT_ENV(buffer, size + 1);

#undef JOBSERVER_PRINT_ENV

  if(ssize == -1) return -1;
  assert(ssize == size);

  return setenv(MAKEFLAGS, buffer, 1);
}
