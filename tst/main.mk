MAKE=/usr/bin/make

all:
	@echo "Error"

TARGETS=jobserver-alone make-to-jobserver jobserver-to-make make-to-jobserver-to-make

.NOTPARALLEL: $(TARGETS)

test: $(TARGETS)

jobserver-alone:
	export JOBSERVER_TEST="jobserver-alone"; ./main.sh one
	export JOBSERVER_TEST="jobserver-alone"; ./main.sh two

make-to-jobserver:
	$(MAKE) -j 5 -f main.mk mtj

mtj:
	+export JOBSERVER_TEST="mtj"; ./main.sh multi

jobserver-to-make:
	+export JOBSERVER_TEST="jobserver-to-make"; ./main 2 $(MAKE) '-f main.mk jtm1'

jtm1:
	./main.sh two
	+./main.sh multi

make-to-jobserver-to-make:
	$(MAKE) -j 6 -f main.mk mtj-jtm

mtj-jtm:
	+JOBSERVER_TEST="mtj-jtm"; ./main ! $(MAKE) '-f main.mk jtm1' '-f main.mk jtm2'

jtm2:
	+./main.sh multi
