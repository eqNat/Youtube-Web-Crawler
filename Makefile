main: main.o bst.o queue.o get_row.o mycurl.o url_conversion.o
	gcc main.o bst.o queue.o get_row.o url_conversion.o mycurl.o -lcurl -pthread -o main

main.o: main.c get_row.h url_conversion.h queue.h bst.h
	gcc -c main.c

get_row.o: get_row.c get_row.h url_conversion.h
	gcc -c get_row.c

mycurl.o: mycurl.c mycurl.h
	gcc -c mycurl.c

url_conversion.o: url_conversion.c url_conversion.h
	gcc -c url_conversion.c

queue.o: queue.c queue.h
	gcc -c queue.c

bst.o: bst.c bst.h
	gcc -c bst.c

clean:
	rm *.o main
