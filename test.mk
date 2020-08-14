all:
	@+echo "MAKEFLAGS:"$(MAKEFLAGS)

test:
	make -f test.mk -j2 test2

test2:
	+make -f test.mk -w all
