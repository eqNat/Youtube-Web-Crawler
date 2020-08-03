crawler: main.o crawler.o lex.yy.o conversions.o panic.o queue.o hash_table.o
	gcc -o $@ $^ -lsqlite3 -lssl -lcrypto -pthread

main.o: main.c queue.h hash_table.h panic.h crawler.h conversions.h
	gcc -c main.c -O3

crawler.o: crawler.c crawler.h json.h queue.h panic.h conversions.h
	gcc -c crawler.c -O3

lex.yy.o: lex.yy.c json.h conversions.h panic.h queue.h hash_table.h
	gcc -c lex.yy.c -lfl -O3

lex.yy.c: json.l
	flex json.l

conversions.o: conversions.c conversions.h panic.h
	gcc -c conversions.c -O3

queue.o: queue.c queue.h
	gcc -c queue.c -O3

hash_table.o: hash_table.c hash_table.h
	gcc -c hash_table.c -O3

panic.o: panic.c panic.h
	gcc -c panic.c -O3

clean:
	rm lex.yy.c *.o
