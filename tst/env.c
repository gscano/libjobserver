#include "jobserver.h"

#include <assert.h> // assert()
#include <stdbool.h> // bool
#include <stdio.h> // printf()
#include <stdlib.h> // EXIT_SUCCESS
#include <string.h> // memcmp()

void read(int id, const char * env, int read, int write, bool dry_run, int ret)
{
  printf("Test #%d\n", id);

  if(env != NULL) assert(setenv("MAKEFLAGS", env, 1) == 0);

  int read_ = -1;
  int write_ = -1;
  bool dry_run_ = false;

  int status = jobserver_getenv(&read_, &write_, &dry_run_);
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

void write(int id, const char * env, int read, int write, bool dry_run, const char * renv)
{
  if(env != NULL)
    {
      assert(setenv("MAKEFLAGS", env, 1) == 0);
    }

  printf("Test #%d\n", id);

  assert(jobserver_setenv(read, write, dry_run) == 0);

  if(memcmp(getenv("MAKEFLAGS"), renv, strlen(getenv("MAKEFLAGS"))) != 0)
    {
      fprintf(stderr, "Incorrect environment: '%s' ('%s' expected)\n",
	      getenv("MAKEFLAGS"), renv);
      assert(false);
    }

  assert(unsetenv("MAKEFLAGS") == 0);
}

int main()
{
  read(1, NULL, -1, -1, false, 0);
  read(2, "", -1, -1, false, 0);
  read(3, "n", -1, -1, true, 0);
  read(4, "eik", -1, -1, false, 0);
  read(5, "-j4 --jobserver-auth=3,4", 3, 4, false, 0);
  read(6, "n -j2 --jobserver-auth=3,4", 3, 4, true, 0);
  read(7, "--jobserv", -1, -1, false, 0);
  read(8, "--jobserver-auth3,4", -1, -1, false, -1);
  read(9, "--jobserver-auth=", -1, -1, false, -1);
  read(10, "--jobserver-auth=3", -1, -1, false, -1);
  read(11, "--jobserver-auth=34", -1, -1, false, -1);
  read(12, "--jobserver-auth=3,", -1, -1, false, -1);
  read(13, "--jobserver-auth=3,4dz", 3, 4, false, 0);
  read(14, "--jobserver-auth=3,4 n", 3, 4, false, 0);
  read(15, "--jobserver-auth=b,4", -1, -1, false, -1);
  read(16, "--jobserver-auth=3,f", -1, -1, false, -1);
  read(17, "n --warn-undefined-variables", -1, -1, true, 0);
  read(18, "--warn-undefined-variables", -1, -1, false, 0);
  read(19, "n --jobserver-auth=-2,4", -1, -1, true, -1);

  write(1, NULL, 3, 4, true, "n --jobserver-auth=3,4");
  write(2, NULL, 3, 4, false, "--jobserver-auth=3,4");
  write(3, "", 3, 4, true, "n --jobserver-auth=3,4");
  write(4, "", 3, 4, false, "--jobserver-auth=3,4");
  write(5, "d", 3, 4, false, "d --jobserver-auth=3,4");
  write(6, "d", 3, 4, true, "nd --jobserver-auth=3,4");
  write(7, "ni", 3, 4, false, "ni --jobserver-auth=3,4");
  write(8, "ni", 3, 4, true, "ni --jobserver-auth=3,4");
  write(9, "-- NAME=VALUE", 3, 4, false, " --jobserver-auth=3,4 -- NAME=VALUE");
  write(10, "in -- NAME=VALUE", 3, 4, false, "in --jobserver-auth=3,4 -- NAME=VALUE");

  return EXIT_SUCCESS;
}
