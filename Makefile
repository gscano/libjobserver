-include config.mk

CFLAGS=-g -O0

all: tst/env tst/init tst/handle

src/%.o: src/%.c
	$(CC) -c $< -o $@ $(CFLAGS) -MD

tst/%.o: tst/%.c
	$(CC) -c $< -o $@ $(CFLAGS) -I src -MD

tst/env: tst/env.o src/env.o
	$(CC) $^ -o $@ $(CFLAGS)

tst/init: tst/init.o src/init.o src/env.o src/internal.o
	$(CC) $^ -o $@ $(CFLAGS)

tst/handle: tst/handle.o src/env.o src/init.o src/handle.o src/token.o src/wait.o src/internal.o src/print.o
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	rm -f src/*.d src/*.o
	rm -f tst/*.d tst/*.o
	rm -f tst/env tst/init tst/handle

distclean: clean
	rm -f `find . -name '*~'`
