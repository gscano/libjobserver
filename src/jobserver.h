#ifndef LIBJOBSERVER_H
#define LIBJOBSERVER_H

#include <stdbool.h>

int jobserver_getenv(int * read_fd, int * write_fd, bool * dry_run);
int jobserver_setenv(int read_fd, int write_fd, bool dry_run);

#endif/*LIBJOBSERVER_H*/
