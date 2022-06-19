all: arm

arm: mini.l mini.y tac.c tac.h obj.c obj.h
	lex -o mini.l.c mini.l
	yacc -d -o mini.y.c mini.y
	gcc -g3 mini.l.c mini.y.c tac.c obj.c -o mini

run: 
	./mini ./sample/assign.m
	gcc -g ./sample/assign.m.s -o ./sample/assign.out
	./sample/assign.out