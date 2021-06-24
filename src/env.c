#include <stdbool.h> // bool
#include <errno.h> // errno
#include <limits.h> // INT_MAX
#include <stdio.h> // snprintf()
#include <stdlib.h> // getenv(), setenv(), malloc(), free(), strtol()
#include <string.h> // strchr(), strstr()

#include "jobserver.h"
#include "config.h"

static inline
char const * jobserver_search_for_options_in_first_word_(char const * env,
							 bool target,
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

char const * jobserver_atofd(char const * str, int * fd)
{
  if(*str < '0' || *str > '9')
    {
      if(str[0] != '\0' && str[0] == '-' && str[1] == '1')
	return str + 2;
      else
	return NULL;
    }

  long int i = 0;

  while('0' <= *str && *str <= '9')
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

int jobserver_read_env(char const * env,
		       int * read_fd, int * write_fd,
		       bool * dry_run)
{
  *read_fd = *write_fd = -1;
  *dry_run = false;

  if(env == NULL) return 0;
  env = jobserver_search_for_options_in_first_word_(env, true, dry_run);

  env = strstr(env, MAKEFLAGS_JOBSERVER);
  if(env == NULL) return 0;
  env = strchr(env, '=');
  if(env == NULL) goto error;
  ++env;

  env = jobserver_atofd(env, read_fd);
  if(env == NULL)
    goto error;

  if(*env++ != ',') goto error;

  env = jobserver_atofd(env, write_fd);
  if(env == NULL)
    goto error;

  return 0;

 error:
  errno = EPROTO;
  *read_fd = *write_fd = -1;

  return -1;
}

char * jobserver_write_env(char const * env,
			   int read_fd, int write_fd,
			   bool dry_run)
{
  char const * before, * j, * fds, * after, * end;

  if(env == NULL) env = before = j = fds = after = end = "";
  else
    {
      before = jobserver_search_for_options_in_first_word_(env, false, &dry_run);

      j = strstr(before, "-j");

      if(j == NULL) j = before;

      fds = strstr(before, MAKEFLAGS_JOBSERVER);

      if(fds == NULL)
	{
	  fds = j;
	  after = j;
	  end = strchr(j, '\0');
	}
      else
	{
	  after = strchr(fds, ' ');
	  end = strchr(fds, '\0');

	  if(after == NULL)
	    after = end;
	}
    }

#define JOBSERVER_PRINT(ptr, size)					\
  snprintf(ptr, size, "%.*s%s",						\
	   (int)(before - env), env,					\
	   dry_run ? "n" : "")

  const int word_size = JOBSERVER_PRINT(NULL, 0);
  char word[word_size + 1];
  JOBSERVER_PRINT(word, word_size + 1);

#undef JOBSERVER_PRINT

#define JOBSERVER_PRINT(ptr, size)					\
  snprintf(ptr, size, "-j "MAKEFLAGS_JOBSERVER"=%d,%d", read_fd, write_fd)

  const bool use_fds = read_fd >= -1 && write_fd >= -1;

  const int jobserver_fds_size = use_fds ? JOBSERVER_PRINT(NULL, 0) : 0;
  char jobserver_fds[jobserver_fds_size + 1];
  if(use_fds)
    JOBSERVER_PRINT(jobserver_fds, jobserver_fds_size + 1);

#undef JOBSERVER_PRINT

  const bool first_space = word_size > 0
    && jobserver_fds_size > 0
    && (!(fds > before) || before[fds - before - 1] != ' ');

  const bool second_space = after < end
    && jobserver_fds_size > 0
    && *after != ' ';

  const int size = word_size
    + first_space + (j - before) + jobserver_fds_size
    + second_space + (end - after);
  char * buffer = malloc((size + 1) * sizeof(char));

  if(buffer == NULL)
    return NULL;// errno: ENOMEM

  const int size_ =
    snprintf(buffer, size + 1, "%s%s%.*s%s%s%s",
	     word,
	     first_space ? " " : "",
	     (int)(j - before), before,
	     jobserver_fds,
	     second_space ? " " : "",
	     after);

  if(size_ != size)
    {
      errno = 0;
      return NULL;
    }

  return buffer;
}

int jobserver_getenv(struct jobserver * js)
{
  return jobserver_read_env(getenv(MAKEFLAGS),
			    &js->poll[1].fd, &js->write,
			    &js->dry_run);
}

int jobserver_setenv_(int read_fd, int write_fd, bool dry_run)
{
  char * const buffer = jobserver_write_env(getenv(MAKEFLAGS),
					    read_fd, write_fd,
					    dry_run);

  if(buffer == NULL)
    return -1;// errno: ENOMEM

  const int ret = setenv(MAKEFLAGS, buffer, 1);// errno: ENOMEM

  free(buffer);

  return ret;
}

int jobserver_setenv(struct jobserver const * js)
{
  return jobserver_setenv_(js->poll[1].fd, js->write,
			   js->dry_run);
}

int jobserver_unsetenv(struct jobserver const * js)
{
  return jobserver_setenv_(-2, -2, js->dry_run);
}
