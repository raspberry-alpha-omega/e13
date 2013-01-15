CFLAGS=-std=c99

e13: obj/e13.o
	gcc -o e13 obj/e13.o
	
obj/e13.o: src/e13.c
	gcc -c $(CFLAGS) -o $@ $<
