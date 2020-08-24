#ifndef LIBJOBSERVER_H
#define LIBJOBSERVER_H

#include <stdbool.h>
#include <stddef.h> // size_t

int jobserver_getenv_(int * read_fd, int * write_fd,
		      bool * dry_run, bool * debug, bool * keep_going);
int jobserver_setenv_(int read_fd, int write_fd,
		      bool dry_run, bool debug, bool keep_going);

#define JOBSERVER_FREE_TOKEN (char)0

struct jobserver
{
  bool dry_run;
  bool debug;
  bool keep_going;

  bool has_free_token;

  int read;
  int write;
};

int jobserver_getenv(struct jobserver * js);
int jobserver_setenv(struct jobserver const * js);
int jobserver_unsetenv(struct jobserver const * js);

int jobserver_connect(struct jobserver * js);
int jobserver_create(struct jobserver * js, size_t size, char token);
int jobserver_create_n(struct jobserver * js, char const * tokens);
int jobserver_close(struct jobserver * js);

#endif/*LIBJOBSERVER_H*/
