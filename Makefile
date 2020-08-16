crawler: main.o crawler.o lex.yy.o conversions.o panic.o dbcache/libcache.a
	gcc -o $@ $^ -lsqlite3 -lssl -lcrypto -pthread

main.o: main.c panic.h crawler.h conversions.h dbcache/queue.h dbcache/hash_table.h
	gcc -c main.c -O3

crawler.o: crawler.c crawler.h json.h panic.h conversions.h dbcache/queue.h dbcache/hash_table.h
	gcc -c crawler.c -O3

lex.yy.o: lex.yy.c
	gcc -c lex.yy.c -lfl -O3

lex.yy.c: json.l json.h conversions.h panic.h dbcache/queue.h dbcache/hash_table.h
	flex json.l

conversions.o: conversions.c conversions.h panic.h
	gcc -c conversions.c -O3

panic.o: panic.c panic.h
	gcc -c panic.c -O3

dbcache/libcache.a:
	$(MAKE) -C dbcache

clean:
	$(MAKE) clean -C dbcache
	rm lex.yy.c *.o
