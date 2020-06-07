main: main.o bst.o queue.o HTML_handler.o base64.o
	gcc main.o bst.o queue.o HTML_handler.o base64.o -lcurl -pthread -o main

main.o: main.c HTML_handler.h base64.h queue.h bst.h
	gcc -c main.c

HTML_handler.o: HTML_handler.c HTML_handler.h base64.h
	gcc -c HTML_handler.c

base64.o: base64.c base64.h
	gcc -c base64.c

queue.o: queue.c queue.h
	gcc -c queue.c

bst.o: bst.c bst.h
	gcc -c bst.c

clean:
	rm *.o main
