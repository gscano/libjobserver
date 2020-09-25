MAKEFLAGS=--no-builtin-rules --no-builtin-variables

.PHONY: all check run-check clean distclean dist install uninstall

BUILDIR:=.
DESTDIR:=/usr
HDR_DESTDIR=$(DESTDIR)/include
LIB_DESTDIR=$(DESTDIR)/lib
MAN_DESTDIR=$(DESTDIR)/share/man

NAME=libjobserver

-include config.mk

all: $(addprefix $(BUILDIR)/$(NAME)-$(VERSION)., a so)

SRC=$(wildcard src/*.c)
OBJ=$(addprefix $(BUILDIR)/, $(SRC:%.c=%.o))

-include $(BUILDIR)/src/%.d

$(BUILDIR)/src/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) -c -fPIC $< $(CFLAGS) -MD -o $@ #-flto

CHECK=env init handle main
check: $(addprefix $(BUILDIR)/tst/, $(CHECK))

-include $(BUILDIR)/tst/%.d

$(BUILDIR)/tst/%.o: tst/%.c
	@mkdir -p $(dir $@)
	$(CC) -c $< $(CFLAGS) -I src -MD -o $@

$(BUILDIR)/tst/%: $(BUILDIR)/tst/%.o $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) $^ $(CFLAGS) -o $@

run-check: check

$(BUILDIR)/$(NAME)-$(VERSION).a: $(OBJ)
	ar -rc $@ $(OBJ)
	ranlib $@

$(BUILDIR)/$(NAME)-$(VERSION).so: $(OBJ)
	$(CC) -shared -Wl,-soname,$@.so -o $@ $^

clean:
	rm -f $(addprefix $(BUILDIR)/, $(NAME).a $(NAME).so)
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
INSTALL+=$(addprefix $(MAN_DESTDIR)/man3/, $(addsuffix .gz, $(shell ./man/script.sh echo | cut -d' ' -f1)))
INSTALL+=$(MAN_DESTDIR)/man7/jobserver.7.gz
install: $(INSTALL) man/script.sh
	@./man/script.sh echo | awk '{print "jobserver__"$$1".gz $(MAN_DESTDIR)/man3/"$$2".gz"}' | xargs -I $$ bash -c "ln -sfv $$"

$(HDR_DESTDIR)/jobserver.h: src/jobserver.h
	@mkdir -p $(dir $@)
	cp $< $@

$(LIB_DESTDIR)/$(NAME)-$(VERSION).%: $(BUILDIR)/$(NAME)-$(VERSION).%
	@mkdir -p $(dir $@)
	cp $< $@

$(MAN_DESTDIR)/man3/%.3.gz: man/%.3
	@mkdir -p $(dir $@)
	@cp $< $(dir $@)jobserver__$(notdir $@)
	gzip --quiet $(dir $@)jobserver__$(notdir $@)

$(MAN_DESTDIR)/man7/jobserver.7.gz: man/jobserver.7
	@mkdir -p $(dir $@)
	@cp $< $@
	gzip --quiet $@

uninstall:
	rm -f $(INSTALL)
