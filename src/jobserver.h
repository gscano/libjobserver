#ifndef LIBJOBSERVER_H
#define LIBJOBSERVER_H

#include <poll.h> // struct pollfd
#include <stdbool.h>
#include <stddef.h> // size_t
#include <stdio.h> // FILE
#include <sys/types.h> // pid_t

struct jobserver
{
  bool dry_run;
  bool debug;
  bool keep_going;

  pid_t stopped;
  int status;

  size_t size;
  bool has_free_token;

  size_t current_jobs;
  size_t max_jobs;
  struct jobserver_job * jobs;

  struct pollfd poll[2];// [SIGCHLD, token pipe (read)]
  int write;// token pipe (write)
};

typedef int (*jobserver_callback_t)(void * data);
typedef void (*jobserver_callback_return_t) (void * data, int status);

int jobserver_getenv(struct jobserver * js);
int jobserver_setenv(struct jobserver const * js);
int jobserver_unsetenv(struct jobserver const * js);

int jobserver_connect(struct jobserver * js);
int jobserver_create(struct jobserver * js, char const * tokens);
int jobserver_create_n(struct jobserver * js, size_t size, char token);
int jobserver_close(struct jobserver * js);

int jobserver_launch_job(struct jobserver * js, int wait, bool shared, void * data,
			 jobserver_callback_t func, jobserver_callback_return_t done);
int jobserver_terminate_job(struct jobserver * js, pid_t pid, int status);

int jobserver_wait(struct jobserver * js, int timeout);
int jobserver_collect(struct jobserver * js, int timeout);
int jobserver_clear(struct jobserver * js);

int jobserver_print(FILE * stream, struct jobserver const * js,
		    const char * separator, const char * job_separator,
		    const char * inter_job_separator);

int jobserver_getenv_(int * read_fd, int * write_fd,
		      bool * dry_run, bool * debug, bool * keep_going);
int jobserver_setenv_(int read_fd, int write_fd,
		      bool dry_run, bool debug, bool keep_going);

void jobserver_close_(struct jobserver * js, bool inherit);

#endif/*LIBJOBSERVER_H*/
