VERSION ?= X.Y.Z
CFLAGS ?= -W -Wall -Werror -Wextra # -DUSE_SIGNALFD # -g -O0
T_CFLAGS ?= #-DNDEBUG -flto
BUILDIR ?= .
OBJDIR ?= $(BUILDIR)
BINDIR ?= $(BUILDIR)
prefix ?= /usr

datarootdir = $(prefix)/share
includedir = $(prefix)/include
libdir = $(prefix)/lib
mandir = $(datarootdir)/man
man3dir = $(mandir)/man3
man7dir = $(mandir)/man7

-include config.mk

MAKEFLAGS=--no-builtin-rules

.PHONY: all check example clean distclean dist install uninstall

all: $(BINDIR)/libjobserver.a $(BINDIR)/libjobserver.so check example

SRC=$(wildcard src/*.c)
OBJ=$(addprefix $(OBJDIR)/, $(SRC:%.c=%.o))

$(BINDIR)/libjobserver.a: $(BINDIR)/libjobserver-$(VERSION).a
	ln -sf $(notdir $<) $@

$(BINDIR)/libjobserver.so: $(BINDIR)/libjobserver-$(VERSION).so
	ln -sf $(notdir $<) $@

$(BINDIR)/libjobserver-$(VERSION).a: $(OBJ)
	@mkdir -p $(dir $@)
	ar -rc $@ $(OBJ)
	ranlib $@

$(BINDIR)/libjobserver-$(VERSION).so: $(OBJ)
	@mkdir -p $(dir $@)
	$(CC) -shared $(LDFLAGS) -o $@ $^

-include $(OBJ:%.o=%.d)

$(OBJDIR)/src/%.o: src/%.c $(OBJDIR)/src/config.h
	@mkdir -p $(dir $@)
	$(CC) -c -fPIC $(CFLAGS) $(T_CFLAGS) -I $(OBJDIR)/src -MMD -o $@ $<

$(OBJDIR)/src/config.h: src/config.h.in
	@mkdir -p $(dir $@)
	$(if $(shell $(MAKE) -j 2 detect-makeflags--jobserver-), echo '#define MAKEFLAGS_JOBSERVER "--jobserver-$(shell $(MAKE) -j 2 detect-makeflags--jobserver-)"' | cat $< - > $@, $(error "Cannot detect MAKEFLAGS's '--jobserver-' section!"))

detect-makeflags--jobserver-:
	@echo "$$MAKEFLAGS" | grep -- '--jobserver-' | sed 's/.*--jobserver-\([a-z]*\)=.*/\1/g'

CHECK=env init handle main
CHECK_OBJ=$(addprefix $(OBJDIR)/tst/, $(CHECK:%=%.o))

.PRECIOUS: $(CHECK_OBJ)
.PRECIOUS: $(addprefix $(BINDIR)/tst/, $(CHECK))

check: $(CHECK_OBJ:$(OBJDIR)/%.o=$(BINDIR)/%.ok)

-include $(CHECK_OBJ:%.o=%.d)

$(OBJDIR)/tst/%.o: tst/%.c $(OBJDIR)/src/config.h
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -I src -I $(OBJDIR)/src -MMD -o $@ $<

$(BINDIR)/tst/%: $(OBJDIR)/tst/%.o $(BINDIR)/libjobserver.a
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

$(BINDIR)/tst/main-so: $(OBJDIR)/tst/main.o $(BINDIR)/libjobserver.so
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) -L $(BINDIR) -o $@ $< -ljobserver

$(BINDIR)/tst/%.ok: $(BINDIR)/tst/%
	$< > $<.ko 2>&1
	$(if $$? 0, mv -f $<.ko $<.ok, rm -f $<.ok; $(error "Test '"$<"' failed!"))

$(BINDIR)/tst/main.ok: $(BINDIR)/tst/main $(BINDIR)/tst/main.mk
	$(MAKE) -j 1 -C $(BINDIR)/tst -f main.mk test 2>$<.stderr 1>$<.ko
	$(if $$? 0, mv -f $<.ko $<.ok, rm -f $<.ok; $(error "Check failed!"))

ifneq ($(BINDIR)/tst/main.mk,./tst/main.mk)
$(BINDIR)/tst/main.mk: tst/main.mk
	cp $< $@
endif

example: xmp/example

-include $(OBJDIR)/xmp/example.d

$(BINDIR)/xmp/example: $(OBJDIR)/xmp/example.o $(BINDIR)/libjobserver.a
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^

$(OBJDIR)/xmp/example.o: xmp/example.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) -I src -MMD -o $@ $<

clean:
	rm -f $(addprefix $(OBJDIR)/, src/config.h)
	rm -f $(addprefix $(BINDIR)/, libjobserver.a libjobserver.so)
	rm -f $(addprefix $(BINDIR)/, libjobserver-$(VERSION).a libjobserver-$(VERSION).so)
	rm -f $(addprefix $(OBJDIR)/src/, *.d *.o)
	rm -f $(addprefix $(OBJDIR)/tst/, *.d *.o)
	rm -f $(addprefix $(BINDIR)/tst/, $(CHECK) $(CHECK:%=%.ko) $(CHECK:%=%.ok) main.stderr main.mk)
	rm -f $(addprefix $(BINDIR)/xmp/, *.d *.o)
	rm -f libjobserver-$(VERSION)

distclean: clean
	rm -f `find . -name '*~'`

DISTFILES=Makefile LICENSE
DISTFILES+=$(wildcard src/*.c) $(wildcard src/*.h) src/config.h.in
DISTFILES+=$(wildcard tst/*.c) tst/main.mk tst/main.sh
DISTFILES+=xmp/example.c
DISTFILES+=$(addprefix man/, script.sh env.3 env_.3 handle.3 handle_.3 init.3 jobserver.7 wait.3)
dist: $(DISTFILES)
	$(foreach file, $^, $(shell mkdir -p $(dir libjobserver-$(VERSION)/$(file))))
	$(foreach file, $^, $(shell cp $(file) libjobserver-$(VERSION)/$(file)))
	tar czfv libjobserver-$(VERSION).tar.gz libjobserver-$(VERSION)
	rm -r libjobserver-$(VERSION)

INSTALL=$(includedir)/jobserver.h
INSTALL+=$(libdir)/libjobserver-$(VERSION).a $(libdir)/libjobserver-$(VERSION).so
INSTALL+=$(libdir)/libjobserver.a $(libdir)/libjobserver.so
INSTALL+=$(addprefix $(man3dir)/jobserver__, $(addsuffix .gz, $(shell ./man/script.sh echo | cut -d' ' -f1)))
INSTALL+=$(man7dir)/jobserver.7.gz
install: $(INSTALL) man/script.sh
	./man/script.sh echo | awk '{print "jobserver__"$$1".gz $(man3dir)/"$$2".gz"}' | xargs -I $$ bash -c "ln -sf $$"

$(includedir)/jobserver.h: src/jobserver.h
	@mkdir -p $(dir $@)
	cp $< $@

$(libdir)/libjobserver.%: $(libdir)/libjobserver-$(VERSION).%
	@mkdir -p $(dir $@)
	ln -sf $(notdir $<) $@

$(libdir)/libjobserver-$(VERSION).%: $(BINDIR)/libjobserver-$(VERSION).%
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
