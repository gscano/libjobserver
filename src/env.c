#include <assert.h> // assert()
#include <stdbool.h> // bool
#include <errno.h> // errno
#include <limits.h> // INT_MAX
#include <stdio.h> // snprintf()
#include <stdlib.h> // getenv(), setenv(), strtol()
#include <string.h> // strchr(), strstr()

#define MAKEFLAGS "MAKEFLAGS"

#define MAKEFLAGS_AUTH "--jobserver-auth"

static inline
bool search_for_char_in_first_word(char const * env, char c)
{
  bool found = false;

  while(*env != '\0' && *env != ' ' && !found)
    found = *env++ == c;

  return found;
}

int jobserver_getenv(int * read_fd, int * write_fd,
		     bool * dry_run, bool * debug, bool * keep_going)
{
  *read_fd = -1;
  *write_fd = -1;
  *dry_run = false;
  *keep_going = false;
  *debug = false;


  char const * env = getenv(MAKEFLAGS);
  if(env == NULL) return 0;

  if(env[0] != '-')
    {
      *dry_run = search_for_char_in_first_word(env, 'n');
      *debug = search_for_char_in_first_word(env, 'd');
      *keep_going = search_for_char_in_first_word(env, 'k');
    }

  char const * fds = strstr(env, MAKEFLAGS_AUTH);
  if(fds == NULL) return 0;
  fds = strchr(fds, '=');
  if(fds == NULL) return -1;
  ++fds;

  long int fd;
  errno = 0;

  env = fds;
  fd = strtol(fds, (char **)&fds, 10);// errno: EINVAL, ERANGE
  if(fds == env || errno || fd < 0 || fd > INT_MAX) goto error;
  *read_fd = (int)fd;

  if(*fds++ != ',') goto error;

  env = fds;
  fd = strtol(fds, (char **)&fds, 10);// errno: EINVAL, ERANGE
  if(fds == env || errno || fd < 0 || fd > INT_MAX) goto error;
  *write_fd = (int)fd;

  return 0;

 error:
  *read_fd = -1;
  *write_fd = -1;
  errno = EBADF;
  return -1;
}

int jobserver_setenv(int read_fd, int write_fd,
		     bool dry_run, bool debug, bool keep_going)
{
  char const * env = getenv(MAKEFLAGS);
  char const * n = "";
  char const * d = "";
  char const * k = "";
  char const * split;
  char const * end;
  bool space = false;

  if(env == NULL)
    {
      env = "";
      if(dry_run) n = "n";
      if(debug) d = "d";
      if(keep_going) k = "k";
      if(dry_run || debug || keep_going) space = true;
      split = end = env;
    }
  else
    {
      end = env + strlen(env);
      if(end != env || dry_run) space = true;

      if(dry_run && !search_for_char_in_first_word(env, 'n')) n = "n";
      if(debug && !search_for_char_in_first_word(env, 'd')) d = "d";
      if(keep_going && !search_for_char_in_first_word(env, 'k')) k = "k";

      split = strstr(env, "-- ");
      if(split == NULL)
	split = end;
    }

  bool space2 = split - 1 >= env && split[-1] == ' ';

#define JOBSERVER_PRINT_ENV(ptr, size)				\
  snprintf(ptr, size, "%s%s%s%.*s%s"MAKEFLAGS_AUTH"=%d,%d%s%s",	\
	   n, d, k, (int)(split - env - space2), env,		\
	   space ? " " : "",					\
	   read_fd, write_fd, split != end ? " " : "", split)

  const int size = JOBSERVER_PRINT_ENV(NULL, 0);

  if(size == -1) return -1;

  char buffer[size + 1];

  const int ssize = JOBSERVER_PRINT_ENV(buffer, size + 1);

#undef JOBSERVER_PRINT_ENV

  if(ssize == -1) return -1;
  assert(ssize == size);

  return setenv(MAKEFLAGS, buffer, 1);// errno: ENOMEM
}
