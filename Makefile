main: main.o bst.o stack.o get_20_rec.o mycurl.o url_conversion.o
	gcc main.o bst.o stack.o get_20_rec.o url_conversion.o mycurl.o -lcurl -pthread -o main

main.o: main.c get_20_rec.h url_conversion.h stack.h bst.h
	gcc -c main.c

get_20_rec.o: get_20_rec.c get_20_rec.h url_conversion.h mycurl.h
	gcc -c get_20_rec.c

mycurl.o: mycurl.c mycurl.h
	gcc -c mycurl.c

url_conversion.o: url_conversion.c url_conversion.h
	gcc -c url_conversion.c

stack.o: stack.c stack.h
	gcc -c stack.c

bst.o: bst.c bst.h
	gcc -c bst.c

clean:
	rm *.o main
