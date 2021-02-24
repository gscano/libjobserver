#include "jobserver.h"
#include "config.h"

#include <assert.h> // assert()
#include <stdbool.h> // bool
#include <stdio.h> // printf()
#include <stdlib.h> // EXIT_SUCCESS
#include <string.h> // memcmp()

void read(int id, const char * env, int read, int write, bool dry_run, int ret)
{
  printf("Test #%d\n", id);

  if(env == NULL) env = "";
  assert(setenv("MAKEFLAGS", env, 1) == 0);

  int read_;
  int write_;
  bool dry_run_;

  int status = jobserver_getenv_(&read_, &write_, &dry_run_);

  if(status != ret)
    {
      fprintf(stderr, "Incorrect function return: %d (%d expected)\t",
	      status, ret);
      assert(false);
    }

  assert(dry_run_ == dry_run);
  assert(read_ == read);
  assert(write_ == write);

  assert(unsetenv("MAKEFLAGS") == 0);
}

void write(int id, const char * env,
	   int read, int write, bool dry_run,
	   const char * renv)
{
  if(env != NULL)
    {
      assert(setenv("MAKEFLAGS", env, 1) == 0);
    }

  printf("Test #%d\n", id);

  assert(jobserver_setenv_(read, write, dry_run) == 0);

  if(memcmp(getenv("MAKEFLAGS"), renv, strlen(renv)) != 0)
    {
      fprintf(stderr, "Incorrect environment: '%s' ('%s' expected)\n",
	      getenv("MAKEFLAGS"), renv);
      assert(false);
    }

  assert(unsetenv("MAKEFLAGS") == 0);
}

int main()
{
  char const * env = getenv("MAKEFLAGS");
  if(env == NULL) env = "";
  char env_[strlen(env) + 1];
  strcpy(env_, env);

  read(1, NULL, -1, -1, false, 0);
  read(2, "", -1, -1, false, 0);
  read(3, "n", -1, -1, true, 0);
  read(4, "eik", -1, -1, false, 0);
  read(5, "-j4 "MAKEFLAGS_JOBSERVER"=3,4", 3, 4, false, 0);
  read(6, "n -j2 "MAKEFLAGS_JOBSERVER"=3,4", 3, 4, true, 0);
  read(7, "--jobserv", -1, -1, false, 0);
  read(8, ""MAKEFLAGS_JOBSERVER"3,4", -1, -1, false, -1);
  read(9, ""MAKEFLAGS_JOBSERVER"=", -1, -1, false, -1);
  read(10, ""MAKEFLAGS_JOBSERVER"=3", -1, -1, false, -1);
  read(11, ""MAKEFLAGS_JOBSERVER"=34", -1, -1, false, -1);
  read(12, ""MAKEFLAGS_JOBSERVER"=3,", -1, -1, false, -1);
  read(13, ""MAKEFLAGS_JOBSERVER"=3,4dz", 3, 4, false, 0);
  read(14, ""MAKEFLAGS_JOBSERVER"=3,4 n", 3, 4, false, 0);
  read(15, ""MAKEFLAGS_JOBSERVER"=b,4", -1, -1, false, -1);
  read(16, ""MAKEFLAGS_JOBSERVER"=3,f", -1, -1, false, -1);
  read(17, "n --warn-undefined-variables", -1, -1, true, 0);
  read(18, "--warn-undefined-variables", -1, -1, false, 0);
  read(19, "n "MAKEFLAGS_JOBSERVER"=-2,4", -1, -1, true, -1);

  write(1, NULL, 3, 4, false, "-j "MAKEFLAGS_JOBSERVER"=3,4");
  write(2, NULL, 3, 4, true, "n -j "MAKEFLAGS_JOBSERVER"=3,4");
  write(4, "", 3, 4, true, "n -j "MAKEFLAGS_JOBSERVER"=3,4");
  write(5, "", 3, 4, false, "-j "MAKEFLAGS_JOBSERVER"=3,4");
  write(6, "d", 3, 4, false, "d -j "MAKEFLAGS_JOBSERVER"=3,4");
  write(7, "d", 3, 4, true, "dn -j "MAKEFLAGS_JOBSERVER"=3,4");
  write(8, "ni", 3, 4, false, "ni -j "MAKEFLAGS_JOBSERVER"=3,4");
  write(10, "-- NAME=VALUE", 3, 4, false, "-j "MAKEFLAGS_JOBSERVER"=3,4 -- NAME=VALUE");
  write(13, "i --long-option -- NAME=VALUE", 3, 4, true,
	"in -j "MAKEFLAGS_JOBSERVER"=3,4 --long-option -- NAME=VALUE");
  write(14, "i --long-option -j4 "MAKEFLAGS_JOBSERVER"=1,2 -- NAME=VALUE", 3, 4, true,
	"in --long-option -j "MAKEFLAGS_JOBSERVER"=3,4 -- NAME=VALUE");
  write(16, "-j4 "MAKEFLAGS_JOBSERVER"=1,2", -1, -1, false, "");
  write(17, "-j4 "MAKEFLAGS_JOBSERVER"=1,2", 3, 4, false, "-j "MAKEFLAGS_JOBSERVER"=3,4");
  write(19, "i -j1 --long-option -- NAME=VALUE", -1, -1, false,
	"i -j1 --long-option -- NAME=VALUE");

  setenv("MAKEFLAGS", env_, 1);

  return EXIT_SUCCESS;
}
