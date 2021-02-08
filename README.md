# Jobserver

A small library to handle parallel jobs in a build automation program compatible with [GNU Make](https://www.gnu.org/software/make/) and under the [GNU LGPL v3](./LICENSE) license.

See the [manual pages](./man/jobserver.7) for documentation.

The code is available on [GitHub](https://github.com/gscano/jobserver) and [code.malloc.fr](https://code.malloc.fr/jobserver).

## Compilation

To compile the library run `make T_CFLAGS=-DNDEBUG`; to install it, run `make install prefix='<path to installation root>'`.

On Linux, it is possible to add `-DUSE_SIGNALFD` to `T_CFLAGS` to use [signalfd(2)](https://manpages.debian.org/stable/manpages-dev/signalfd.2.html). Otherwise, the self-pipe trick is used.

A `config.mk` file can be used to conveniently set up compilation options.

### Environment variable `--jobserver-...`

The build process will infer, from the version of GNU Make which is used to build the library, the environment variable that should be used (`--jobserver-fds` before version 4.2 or `--jobserver-auth` for version 4.2 and after). It is however possible to edit `$(BUILDIR)/src/config.h` to manually select a value for `MAKEFLAGS_JOBSERVER`.

### Testing

To test the library with different versions of GNU Make, run `./install-make V` where `V` is an existing release identifier. Then run `./make/make-V/make check` to compile and test the library against this specific version `V` of GNU Make.

## Contributing

Contributions are most welcome, especially to support more platforms.