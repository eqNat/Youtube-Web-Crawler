main: main.o bst.o queue.o HTML_handler.o url_conversion.o
	gcc main.o bst.o queue.o HTML_handler.o url_conversion.o -lcurl -pthread -o main

main.o: main.c HTML_handler.h url_conversion.h queue.h bst.h
	gcc -c main.c

HTML_handler.o: HTML_handler.c HTML_handler.h url_conversion.h
	gcc -c HTML_handler.c

url_conversion.o: url_conversion.c url_conversion.h
	gcc -c url_conversion.c

queue.o: queue.c queue.h
	gcc -c queue.c

bst.o: bst.c bst.h
	gcc -c bst.c

clean:
	rm *.o main
