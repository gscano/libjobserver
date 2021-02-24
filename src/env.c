#include <stdbool.h> // bool
#include <errno.h> // errno
#include <limits.h> // INT_MAX
#include <stdio.h> // snprintf()
#include <stdlib.h> // getenv(), setenv(), malloc(), free(), strtol()
#include <string.h> // strchr(), strstr()

#include "jobserver.h"
#include "config.h"

static inline
char const * search_for_options_in_first_word_(char const * env, bool target,
					       bool * dry_run)
{
  if(env[0] != '-')
    {
      while(*env != '\0' && *env != ' ')
	{
	  switch(*env)
	    {
	    case 'n': *dry_run = target; break;
	    default : ;
	    }

	  ++env;
	}
    }

  return env;
}

static inline
char const * atofd(char const * str, int * fd)
{
  if(*str < '0' || *str > '9')
    {
      if(str[0] != '\0' && str[0] == '-' && str[1] == '1')
	return str + 2;
      else
	return NULL;
    }

  long int i = 0;

  while('0' < *str && *str < '9')
    {
      i *= 10;
      i += *str - '0';
      ++str;

      if(i > INT_MAX)
	return NULL;
    }

  *fd = (int)i;

  return str;
}

int jobserver_getenv_(int * read_fd, int * write_fd, bool * dry_run)
{
  *read_fd = *write_fd = -1;
  *dry_run = false;

  char const * env = getenv(MAKEFLAGS);

  if(env == NULL) return 0;
  env = search_for_options_in_first_word_(env, true, dry_run);

  env = strstr(env, MAKEFLAGS_JOBSERVER);
  if(env == NULL) return 0;
  env = strchr(env, '=');
  if(env == NULL) goto error;
  ++env;

  env = atofd(env, read_fd);
  if(env == NULL)
    goto error;

  if(*env++ != ',') goto error;

  env = atofd(env, write_fd);
  if(env == NULL)
    goto error;

  return 0;

 error:
  errno = EPROTO;
  *read_fd = *write_fd = -1;

  return -1;
}

int jobserver_setenv_(int read_fd, int write_fd, bool dry_run)
{
  char const * env, * word_end, * before, * j, * fds, * after, * end;

  env = getenv(MAKEFLAGS);

  if(env == NULL) env = word_end = before = j = fds = after = end = "";
  else
    {
      word_end = search_for_options_in_first_word_(env, false, &dry_run);

      before = word_end;
      j = strstr(word_end, "-j");

      if(j == NULL) j = word_end;

      fds = strstr(word_end, MAKEFLAGS_JOBSERVER);

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
  snprintf(ptr, size, "%.*s%s",						\
	   (int)(word_end - env), env,					\
	   dry_run ? "n" : "")

  const int word_size = JOBSERVER_PRINT(NULL, 0);
  char word[word_size + 1];
  JOBSERVER_PRINT(word, word_size + 1);

#undef JOBSERVER_PRINT

#define JOBSERVER_PRINT(ptr, size)					\
  snprintf(ptr, size, "-j "MAKEFLAGS_JOBSERVER"=%d,%d", read_fd, write_fd)

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

	   (int)(j - before), before,

	   read_fd >= 0 && write_fd >= 0 ? jobserver_auth : "",

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
  return jobserver_getenv_(&js->poll[1].fd, &js->write, &js->dry_run);
}

int jobserver_setenv(struct jobserver const * js)
{
  return jobserver_setenv_(js->poll[1].fd, js->write, js->dry_run);
}

int jobserver_unsetenv(struct jobserver const * js)
{
  return jobserver_setenv_(-1, -1, js->dry_run);
}
