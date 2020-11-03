all:
	@+echo "Error"

called-1:
	@+echo "called-1"
	@./main.sh -
	@./main.sh +

called-2:
	@+echo "called-2"
	@+./main.sh %

called-3:
	@+echo "called-3"
	@./main.sh &

call-1:
	./main 2 ./main.sh @

call-2:
	+./main 1 ./main.sh + - @
