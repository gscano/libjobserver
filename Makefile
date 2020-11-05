MAKEFLAGS=--no-builtin-rules --no-builtin-variables

-include config.mk
VERSION?=X.X.X
CC?=gcc
CFLAGS?=-W -Wall -Werror -Wextra -g -O0 # -DUSE_SIGNALFD
T_CFLAGS?= #-DNDEBUG
BUILDIR?=.
DESTDIR?=/usr
HDR_DESTDIR?=$(DESTDIR)/include
LIB_DESTDIR?=$(DESTDIR)/lib
MAN_DESTDIR?=$(DESTDIR)/share/man

MAKEFLAGS=--no-builtin-rules --no-builtin-variables

.PHONY: all check run-check example clean distclean dist install uninstall

NAME=libjobserver

all: $(BUILDIR)/$(NAME).a $(BUILDIR)/$(NAME).so

SRC=$(wildcard src/*.c)
OBJ=$(addprefix $(BUILDIR)/, $(SRC:%.c=%.o))

$(BUILDIR)/$(NAME).a: $(BUILDIR)/$(NAME)-$(VERSION).a
	ln -sf $(notdir $<) $@

$(BUILDIR)/$(NAME).so: $(BUILDIR)/$(NAME)-$(VERSION).so
	ln -sf $(notdir $<) $@

$(BUILDIR)/$(NAME)-$(VERSION).a: $(OBJ)
	ar -rc $@ $(OBJ)
	ranlib $@

$(BUILDIR)/$(NAME)-$(VERSION).so: $(OBJ)
	$(CC) -shared -Wl,-soname,$@ -o $@ $^

-include $(OBJ:%.o=%.d)

$(BUILDIR)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -c -fPIC -flto $(CFLAGS) $(T_CFLAGS) -MMD -o $@ $<

CHECK=env init handle
CHECK_OBJ=$(addprefix $(BUILDIR)/tst/, $(CHECK:%=%.o))

.PRECIOUS: $(CHECK_OBJ)
.PRECIOUS: $(addprefix $(BUILDIR)/tst/, $(CHECK))

check: $(CHECK_OBJ:%.o=%.ok) run-check

run-check: $(BUILDIR)/tst/main
	$(MAKE) -C ./tst -f test.mk

-include $(CHECK_OBJ:%.o=%.d)

$(BUILDIR)/tst/%.o: tst/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -I src -MMD -o $@ $<

$(BUILDIR)/tst/%: $(BUILDIR)/tst/%.o $(BUILDIR)/$(NAME).a
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILDIR)/tst/main-so: $(BUILDIR)/tst/main.o $(BUILDIR)/$(NAME).so
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -L $(BUILDIR) -o $@ $< -ljobserver

$(BUILDIR)/tst/%.ok: $(BUILDIR)/tst/%
	$< > $<.ko 2>&1
	$(if $$? 0, mv -f -u $<.ko $<.ok, rm -f $<.ok; $(error "Test '"$<"' failed!"))

example: exp/example

-include $(BUILDIR)/exp/example.d

$(BUILDIR)/exp/example: $(BUILDIR)/exp/example.o $(BUILDIR)/$(NAME).a
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILDIR)/exp/example.o: exp/example.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -I src -MMD -o $@ $<

clean:
	rm -f $(addprefix $(BUILDIR)/, $(NAME).a $(NAME).so)
	rm -f $(addprefix $(BUILDIR)/, $(NAME)-$(VERSION).a $(NAME)-$(VERSION).so)
	rm -f $(addprefix $(BUILDIR)/src/, *.d *.o)
	rm -f $(addprefix $(BUILDIR)/tst/, *.d *.o)
	rm -f $(addprefix $(BUILDIR)/tst/, $(CHECK) $(CHECK:%=%.ko) $(CHECK:%=%.ok))
	rm -f $(BUILDIR)/tst/main

distclean: clean
	rm -f `find . -name '*~'`

DISTFILES=Makefile LICENSE
DISTFILES+=$(wildcard src/*.c) $(wildcard src/*.h)
DISTFILES+=$(wildcard tst/*.c) tst/main.sh tst/main.mk tst/test.mk
DISTFILES+=exp/example.c
DISTFILES+=$(addprefix man/, script.sh env.3 env_.3 handle.3 handle_.3 init.3 jobserver.7 wait.3)
dist: $(DISTFILES)
	$(foreach file, $^, $(shell mkdir -p $(dir $(NAME)-$(VERSION)/$(file))))
	$(foreach file, $^, $(shell cp $(file) $(NAME)-$(VERSION)/$(file)))
	tar czfv $(NAME)-0.1.0.tar.gz $(NAME)-$(VERSION)
	rm -r $(NAME)-$(VERSION)

INSTALL=$(HDR_DESTDIR)/jobserver.h
INSTALL+=$(LIB_DESTDIR)/$(NAME)-$(VERSION).a $(LIB_DESTDIR)/$(NAME)-$(VERSION).so
INSTALL+=$(LIB_DESTDIR)/$(NAME).a $(LIB_DESTDIR)/$(NAME).so
INSTALL+=$(addprefix $(MAN_DESTDIR)/man3/jobserver__, $(addsuffix .gz, $(shell ./man/script.sh echo | cut -d' ' -f1)))
INSTALL+=$(MAN_DESTDIR)/man7/jobserver.7.gz
install: $(INSTALL) man/script.sh
	./man/script.sh echo | awk '{print "jobserver__"$$1".gz $(MAN_DESTDIR)/man3/"$$2".gz"}' | xargs -I $$ bash -c "ln -sf $$"

$(HDR_DESTDIR)/jobserver.h: src/jobserver.h
	@mkdir -p $(dir $@)
	cp $< $@

$(LIB_DESTDIR)/$(NAME).%: $(LIB_DESTDIR)/$(NAME)-$(VERSION).%
	@mkdir -p $(dir $@)
	ln -sf $(notdir $<) $@

$(LIB_DESTDIR)/$(NAME)-$(VERSION).%: $(BUILDIR)/$(NAME)-$(VERSION).%
	@mkdir -p $(dir $@)
	cp $< $@

$(MAN_DESTDIR)/man3/jobserver__%.3.gz: man/%.3
	@mkdir -p $(dir $@)
	@cp $< $@
	gzip --quiet $@

$(MAN_DESTDIR)/man7/jobserver.7.gz: man/jobserver.7
	@mkdir -p $(dir $@)
	@cp $< $@
	gzip --quiet $@

uninstall:
	rm -f $(INSTALL)
	./man/script.sh echo | awk '{print "jobserver__"$$1".gz $(MAN_DESTDIR)/man3/"$$2".gz"}' | xargs -I $$ bash -c "rm -f $$"
