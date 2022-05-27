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
  bool is_connected;

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

typedef int (*jobserver_main_callback_t)(void * arg);
typedef void (*jobserver_exit_callback_t) (void * data, size_t id, int status);

int jobserver_read_env(char const * env,
		       int * read_fd, int * write_fd, bool * dry_run);
char * jobserver_write_env(char const * env,
			   int read_fd, int write_fd, bool dry_run);

int jobserver_getenv(struct jobserver * js);
int jobserver_setenv(struct jobserver const * js);
int jobserver_unsetenv(struct jobserver const * js);

int jobserver_connect(struct jobserver * js);
int jobserver_connect_to(struct jobserver * js, int read, int write, bool dry_run);
int jobserver_reconnect(struct jobserver * js);
int jobserver_create(struct jobserver * js, char const * tokens, bool dry_run);
int jobserver_create_n(struct jobserver * js, size_t size, char token, bool dry_run);
int jobserver_close(struct jobserver * js);

int jobserver_launch_job(struct jobserver * js, int wait, bool shared,
			 void * arg, void * data, size_t id,
			 jobserver_main_callback_t main,
			 jobserver_exit_callback_t exit);
int jobserver_terminate_job(struct jobserver * js, pid_t pid, int status);

int jobserver_wait(struct jobserver * js, int timeout);
int jobserver_collect(struct jobserver * js, int timeout);
void jobserver_clear(struct jobserver * js);

int jobserver_print(FILE * stream, struct jobserver const * js,
		    const char * separator, const char * job_separator,
		    const char * inter_job_separator);

void jobserver_close_(struct jobserver * js, bool inherit);

#endif/*LIBJOBSERVER_H*/
