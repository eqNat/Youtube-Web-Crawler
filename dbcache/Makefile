libcache.a: hash_table.o queue.o
	ar rs -o libcache.a hash_table.o queue.o

queue.o: queue.c queue.h
	gcc -c queue.c

hash_table.o: hash_table.c hash_table.h
	gcc -c hash_table.c

clean:
	rm libcache.a *.o
