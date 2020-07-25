json: main.o lex.yy.o conversions.o panic.o
	gcc -o json main.o lex.yy.o conversions.o panic.o -lssl -lcrypto

main.o: main.c json.h
	gcc -c main.c

lex.yy.o: json.l json.h conversions.h panic.h
	flex json.l
	gcc -c lex.yy.c -lfl
	rm lex.yy.c

conversions.o: conversions.c conversions.h panic.h
	gcc -c conversions.c

panic.o: panic.c panic.h
	gcc -c panic.c

clean:
	rm json *.o
