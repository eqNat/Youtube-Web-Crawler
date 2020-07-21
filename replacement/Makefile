json: main.o lex.yy.o conversions.o
	gcc -o json main.o lex.yy.o conversions.o

main.o: main.c json.h
	gcc -c main.c

lex.yy.o: lex.yy.c conversions.h
	gcc -c lex.yy.c -lfl

lex.yy.c: json.l json.h
	flex json.l

conversions.o: conversions.c conversions.h
	gcc -c conversions.c

clean:
	rm json lex.yy.c *.o
