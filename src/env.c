#include <assert.h> // assert()
#include <stdbool.h> // bool
#include <errno.h> // errno
#include <limits.h> // INT_MAX
#include <stdio.h> // snprintf()
#include <stdlib.h> // getenv(), setenv(), malloc(), free(), strtol()
#include <string.h> // strchr(), strstr()

#include "jobserver.h"

#define MAKEFLAGS "MAKEFLAGS"

#define MAKEFLAGS_AUTH "--jobserver-auth"

static inline
char const * search_for_options_in_first_word(char const * env, bool target,
					      bool * dry_run, bool * debug, bool * keep_going)
{
  if(env[0] != '-')
    {
      while(*env != '\0' && *env != ' ')
	{
	  switch(*env)
	    {
	    case 'n': *dry_run = target; break;
	    case 'd': *debug = target; break;
	    case 'k': *keep_going = target; break;
	    default : ;
	    }

	  ++env;
	}
    }

  return env;
}

int jobserver_getenv_(int * read_fd, int * write_fd,
		      bool * dry_run, bool * debug, bool * keep_going)
{
  *read_fd = *write_fd = -1;
  *dry_run = *keep_going = *debug = false;

  char const * env = getenv(MAKEFLAGS);
  if(env == NULL) return 0;

  search_for_options_in_first_word(env, true, dry_run, debug, keep_going);

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
  *read_fd = *write_fd = -1;
  errno = EBADF;
  return -1;
}

int jobserver_setenv_(int read_fd, int write_fd,
		      bool dry_run, bool debug, bool keep_going)
{
  char const * env, * word_end, * before, * j, * fds, * after, * end;

  env = getenv(MAKEFLAGS);

  if(env == NULL) env = word_end = before = j = fds = after = end = "";
  else
    {
      word_end = search_for_options_in_first_word(env, false,
						  &dry_run, &debug, &keep_going);

      before = word_end;
      j = strstr(word_end, "-j");

      if(j == NULL) j = word_end;

      fds = strstr(word_end, MAKEFLAGS_AUTH);

      if(fds == NULL)
	{
	  fds = j;
	  after = j;
	}
      else
	{
	  after = strchr(fds, ' ');

	  if(after == NULL) after = word_end;
	}

      end = strchr(after, '\0');
    }

  bool j1 = read_fd < 0 && write_fd < 0;

#define JOBSERVER_PRINT(ptr, size)					\
  snprintf(ptr, size, "%.*s%s%s%s",					\
	   word_end - env, env,						\
	   dry_run ? "n" : "", debug ? "d" : "", keep_going ? "k" : "")

  const int word_size = JOBSERVER_PRINT(NULL, 0);
  char word[word_size + 1];
  JOBSERVER_PRINT(word, word_size + 1);

#undef JOBSERVER_PRINT

#define JOBSERVER_PRINT(ptr, size)					\
  snprintf(ptr, size, MAKEFLAGS_AUTH"=%d,%d", read_fd, write_fd)

  const int jobserver_auth_size = j1 ? 0 : JOBSERVER_PRINT(NULL, 0);
  char jobserver_auth[jobserver_auth_size + 1];
  if(!j1)
    JOBSERVER_PRINT(jobserver_auth, jobserver_auth_size + 1);

#undef JOBSERVER_PRINT

  const int size = word_size + 1 + (fds - before) + jobserver_auth_size + (end - after) + 1;
  char * buffer = malloc((size + 1) * sizeof(char));
  snprintf(buffer, size + 1, "%s%s%.*s%s%s%s", word,

	   word_size > 0
	   && jobserver_auth_size > 0
	   && (!(fds > before) || before[fds - before - 1] != ' ') ? " " : "",

	   j - before, before,

	   read_fd > 0 && write_fd > 0 ? jobserver_auth : "",

	   after < end
	   && jobserver_auth_size > 0
	   && *after != ' ' ? " " : "",

	   after);

  int ret = setenv(MAKEFLAGS, buffer, 1);// errno: ENOMEM

  free(buffer);

  return ret;
}

int jobserver_getenv(struct jobserver * js)
{
  return jobserver_getenv_(&js->read, &js->write,
			   &js->dry_run, &js->debug, &js->keep_going);
}

int jobserver_setenv(struct jobserver const * js)
{
  return jobserver_setenv_(js->read, js->write,
			   js->dry_run, js->debug, js->keep_going);
}

int jobserver_unsetenv(struct jobserver const * js)
{
  return jobserver_setenv_(-1, -1,
			   js->dry_run, js->debug, js->keep_going);
}
