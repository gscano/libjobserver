# Jobserver

A small library to handle parallel jobs in a build automation program compatible with [GNU Make](https://www.gnu.org/software/make/) version 4.2 and higher, and under the [GNU LGPL v3](./LICENSE) license.

See the [manual pages](./man/jobserver.7) for the documentation.

The code is available on [GitHub](https://github.com/gscano/jobserver) and [code.malloc.fr](https://code.malloc.fr/jobserver).

## Compilation

To compile the library run `make T_CFLAGS=-NDEBUG`.
To install it, run `make install prefix='<path to installation root>'`.

On Linux, it is possible to add `-DUSE_SIGNALFD` to `T_CFLAGS` to use [signalfd(2)](https://manpages.debian.org/stable/manpages-dev/signalfd.2.html). Otherwise, the self-pipe trick is used.

A `config.mk` file can be used to conveniently set up compilation options.

## Configuration

The library is compatible with GNU Make version 4.2 and higher.

## Contributing

Contributions are most welcome, especially to support more platforms and older versions of GNU Make.