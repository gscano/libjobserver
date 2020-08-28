#include "jobserver.h"
#include "internal.h"

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
