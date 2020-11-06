VERSION ?= X.Y.Z
CFLAGS ?= -W -Wall -Werror -Wextra -g -O0 # -DUSE_SIGNALFD
T_CFLAGS ?= #-DNDEBUG -flto
BUILDIR ?= .
prefix ?= /usr

datarootdir = $(prefix)/share
includedir = $(prefix)/include
libdir = $(prefix)/lib
mandir = $(datarootdir)/man
man3dir = $(mandir)/man3
man7dir = $(mandir)/man7

#-include config.mk

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
	$(CC) -shared $(LDFLAGS) -o $@ $^

-include $(OBJ:%.o=%.d)

$(BUILDIR)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -c -fPIC $(CFLAGS) $(T_CFLAGS) -MMD -o $@ $<

CHECK=env init handle main
CHECK_OBJ=$(addprefix $(BUILDIR)/tst/, $(CHECK:%=%.o))

.PRECIOUS: $(CHECK_OBJ)
.PRECIOUS: $(addprefix $(BUILDIR)/tst/, $(CHECK))

check: $(CHECK_OBJ:%.o=%.ok)

-include $(CHECK_OBJ:%.o=%.d)

$(BUILDIR)/tst/%.o: tst/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -I src -MMD -o $@ $<

$(BUILDIR)/tst/%: $(BUILDIR)/tst/%.o $(BUILDIR)/$(NAME).a
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILDIR)/tst/main-so: $(BUILDIR)/tst/main.o $(BUILDIR)/$(NAME).so
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -L $(BUILDIR) -o $@ $< -ljobserver

$(BUILDIR)/tst/%.ok: $(BUILDIR)/tst/%
	$< > $<.ko 2>&1
	$(if $$? 0, mv -f $<.ko $<.ok, rm -f $<.ok; $(error "Test '"$<"' failed!"))

$(BUILDIR)/tst/main.ok: $(BUILDIR)/tst/main
	$(MAKE) --no-print-directory -j 1 -C ./tst -f main.mk test #2>$<.ko 1>$<.ok

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
DISTFILES+=$(wildcard tst/*.c) tst/main.sh tst/main.mk
DISTFILES+=exp/example.c
DISTFILES+=$(addprefix man/, script.sh env.3 env_.3 handle.3 handle_.3 init.3 jobserver.7 wait.3)
dist: $(DISTFILES)
	$(foreach file, $^, $(shell mkdir -p $(dir $(NAME)-$(VERSION)/$(file))))
	$(foreach file, $^, $(shell cp $(file) $(NAME)-$(VERSION)/$(file)))
	tar czfv $(NAME)-0.1.0.tar.gz $(NAME)-$(VERSION)
	rm -r $(NAME)-$(VERSION)

INSTALL=$(includedir)/jobserver.h
INSTALL+=$(libdir)/$(NAME)-$(VERSION).a $(libdir)/$(NAME)-$(VERSION).so
INSTALL+=$(libdir)/$(NAME).a $(libdir)/$(NAME).so
INSTALL+=$(addprefix $(man3dir)/jobserver__, $(addsuffix .gz, $(shell ./man/script.sh echo | cut -d' ' -f1)))
INSTALL+=$(man7dir)/jobserver.7.gz
install: $(INSTALL) man/script.sh
	./man/script.sh echo | awk '{print "jobserver__"$$1".gz $(man3dir)/"$$2".gz"}' | xargs -I $$ bash -c "ln -sf $$"

$(includedir)/jobserver.h: src/jobserver.h
	@mkdir -p $(dir $@)
	cp $< $@

$(libdir)/$(NAME).%: $(libdir)/$(NAME)-$(VERSION).%
	@mkdir -p $(dir $@)
	ln -sf $(notdir $<) $@

$(libdir)/$(NAME)-$(VERSION).%: $(BUILDIR)/$(NAME)-$(VERSION).%
	@mkdir -p $(dir $@)
	cp $< $@

$(man3dir)/jobserver__%.3.gz: man/%.3
	@mkdir -p $(dir $@)
	@cp $< $@
	gzip --quiet $@

$(man7dir)/jobserver.7.gz: man/jobserver.7
	@mkdir -p $(dir $@)
	@cp $< $@
	gzip --quiet $@

uninstall:
	rm -f $(INSTALL)
	./man/script.sh echo | awk '{print "jobserver__"$$1".gz $(man3dir)/"$$2".gz"}' | xargs -I $$ bash -c "rm -f $$"
