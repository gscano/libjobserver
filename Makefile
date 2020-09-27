MAKEFLAGS=--no-builtin-rules --no-builtin-variables

.PHONY: all check run-check clean distclean dist install uninstall

-include config.mk
VERSION?=X.X.X
CC?=gcc
CFLAGS?=-W -Wall -Werror -Wextra -g -O0 #-DUSE_SIGNALFD
BUILDIR?=.
DESTDIR?=/usr
HDR_DESTDIR?=$(DESTDIR)/include
LIB_DESTDIR?=$(DESTDIR)/lib
MAN_DESTDIR?=$(DESTDIR)/share/man

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

-include $(BUILDIR)/src/%.d

$(BUILDIR)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -c -fPIC -flto $(CFLAGS) -MD -o $@ $<

CHECK=env init handle main
check: $(addprefix $(BUILDIR)/tst/, $(CHECK))

-include $(BUILDIR)/tst/%.d

$(BUILDIR)/tst/%.o: tst/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -I src -MD -o $@ $<

$(BUILDIR)/tst/%: $(BUILDIR)/tst/%.o $(BUILDIR)/$(NAME).a
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILDIR)/tst/main: $(BUILDIR)/tst/main.o $(BUILDIR)/$(NAME).so
	@mkdir -p $(dir $@)
	$(CC) -L $(BUILDIR) -o $@ $< -ljobserver

run-check: check

clean:
	rm -f $(addprefix $(BUILDIR)/, $(NAME).a $(NAME).so)
	rm -f $(addprefix $(BUILDIR)/, $(NAME)-$(VERSION).a $(NAME)-$(VERSION).so)
	rm -f $(addprefix $(BUILDIR)/src/, *.d *.o)
	rm -f $(addprefix $(BUILDIR)/tst/, *.d *.o)
	rm -f $(addprefix $(BUILDIR)/tst/, $(CHECK))

distclean: clean
	rm -f `find . -name '*~'`

DISTFILES=Makefile LICENSE
DISTFILES+=$(wildcard src/*.c) $(wildcard src/*.h)
DISTFILES+=$(wildcard tst/*.c) tst/main.sh
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
