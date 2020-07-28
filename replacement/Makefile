json: main.o lex.yy.o conversions.o panic.o queue.o hash_table.o
	gcc -o json $^ -lsqlite3 -lssl -lcrypto

main.o: main.c json.h queue.h hash_table.h panic.h
	gcc -c main.c

lex.yy.o: json.l json.h conversions.h panic.h queue.h hash_table.h
	flex json.l
	gcc -c lex.yy.c -lfl
	rm lex.yy.c

conversions.o: conversions.c conversions.h panic.h
	gcc -c conversions.c

queue.o: queue.c queue.h
	gcc -c queue.c

hash_table.o: hash_table.c hash_table.h
	gcc -c hash_table.c

panic.o: panic.c panic.h
	gcc -c panic.c

clean:
	rm json *.o
