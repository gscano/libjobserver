TARGETS=jobserver-alone make-to-jobserver jobserver-to-make make-to-jobserver-to-make

.NOTPARALLEL: $(TARGETS)

all: $(TARGETS)

jobserver-alone:
	@make -f main.mk j

make-to-jobserver:
	@make -j 5 -f main.mk mtj-1

jobserver-to-make:
	@./main.sh %

make-to-jobserver-to-make:
	@make -j 6 -f main.mk mtj-jtm

test1:
	@+echo "MAKEFLAGS:"$(MAKEFLAGS)
	@make -f test.mk -j2 manual-test2 --warn-undefined-variables -k

test2:
	@+echo "MAKEFLAGS:"$(MAKEFLAGS)
	@+bash manual-test.sh -w all

test3:
	@+echo "MAKEFLAGS:"$(MAKEFLAGS)
	@make -f test.mk -I .. manual-test1 -- NAME=VALUE
