#include "jobserver.h"
#include "internal.h"

int acquire_jobserver_token_(struct jobserver * js, int wait, char * token)
{
  if(js->has_free_token)
    {
      *token = JOBSERVER_FREE_TOKEN;
      js->has_free_token = false;

      return 1;
    }
  else
    {
      return jobserver_wait_(js, wait, token);// errno: ECHILD, EINTR, ENOMEM
    }
}

int release_jobserver_token_(struct jobserver * js, char token)
{
  if(token == JOBSERVER_FREE_TOKEN)
    {
      js->has_free_token = true;
    }
  else
    {
      // Ignore errors, someone messed up the pipe anyway
      write_to_pipe_(js->write, &token, 1);
    }

  return 1;
}
