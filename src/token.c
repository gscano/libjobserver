#include "jobserver.h"

// internal.c
extern int write_to_pipe(int fd, const char * buf, size_t count);
extern int read_from_pipe(int fd, char * token);

#define JOBSERVER_FREE_TOKEN (char)0

int acquire_jobserver_token(struct jobserver * js, char * token)
{
  if(js->has_free_token)
    {
      *token = JOBSERVER_FREE_TOKEN;
      js->has_free_token = false;

      return 0;
    }
  else
    {
      return read_from_pipe(js->read, token);
    }
}

int release_jobserver_token(struct jobserver * js, char token)
{
  if(token == JOBSERVER_FREE_TOKEN)
    {
      js->has_free_token = true;

      return 0;
    }
  else
    {
      return write_to_pipe(js->write, &token, 1);
    }
}
