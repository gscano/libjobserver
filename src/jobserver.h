#ifndef LIBJOBSERVER_H
#define LIBJOBSERVER_H

#include <stdbool.h>

int jobserver_getenv(int * read_fd, int * write_fd,
		     bool * dry_run, bool * debug, bool * keep_going);
int jobserver_setenv(int read_fd, int write_fd,
		     bool dry_run, bool debug, bool keep_going);

#endif/*LIBJOBSERVER_H*/
