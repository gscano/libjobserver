all:
	@echo "Error"

j:
	@./main.sh one1
	@./main.sh two1

mtj-1:
	@+./main.sh multi1

jtm-1:
	@./main.sh two1
	@+./main.sh multi1

jtm-2:
	@+./main.sh multi1

mtj-jtm:
	@+./main.sh @
