MAKE=/usr/bin/make

F=-f main.mk
W=$(F) w.
E=export JOBSERVER_TEST=$$JOBSERVER_TEST

all:
	@echo "Error"

TARGETS=make-to-jobserver jobserver-to-make make-to-jobserver-to-make

.NOTPARALLEL: $(TARGETS)

test: $(TARGETS)

make-to-jobserver:
	$(E)/$@; $(MAKE) -j 5 $(F) one two multi

jobserver-to-make:
	+$(E)/$@; ./main 2 $(MAKE) '$(F) jtm1'

jtm1:
	$(E)/$@; $(MAKE) $(F) two
	+$(E)/$@; $(MAKE) $(F) multi

make-to-jobserver-to-make:
	$(MAKE) -j 6 $(F) mtj-jtm

mtj-jtm:
	+$(E)/"$@"; ./main ! $(MAKE) '$(F) jtm1' '$(F) jtm2'

jtm2:
	$(E)/$@; $(MAKE) $(F) one

one:
	$(E)/$@/; ./main 0 $(MAKE) '$(W)1' '$(W)2' '$(W)1' '$(W)2'

two:
	$(E)/$@/; ./main 1 $(MAKE) '$(W)1' '$(W)2' '$(W)1' '$(W)2' '$(W)1' '$(W)2'

multi:
	+$(E)/$@/; ./main ! $(MAKE) '$(W)1' '$(W)2' '$(W)1' '$(W)2' '$(W)1' '$(W)2' '$(W)1' '$(W)2'

w.%:
	@echo "$$JOBSERVER_TEST"
	@sleep $(@:w.%=%)
