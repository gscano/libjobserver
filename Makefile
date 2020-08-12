-include config.mk

CFLAGS=-g -O0

all: src/env.o tst/env

src/%.o: src/%.c
	$(CC) -c $< -o $@ $(CFLAGS) -MD

tst/%.o: tst/%.c
	$(CC) -c $< -o $@ $(CFLAGS) -I src -MD

tst/env: tst/env.o src/env.o
	$(CC) $^ -o $@ $(CFLAGS)

clean:
	rm -f src/env.d src/env.o
	rm -f tst/env.d tst/env.o tst/env

distclean: clean
	rm -f `find . -name '*~'`
