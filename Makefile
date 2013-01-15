CFLAGS=-std=c99

e13: obj/e13.o obj/io.o obj/debug.o
	gcc -o e13 obj/e13.o obj/io.o obj/debug.o
	
obj/e13.o: src/e13.c
	gcc -c $(CFLAGS) -o $@ $<
	
obj/io.o: src/io.c
	gcc -c $(CFLAGS) -o $@ $<
		
obj/debug.o: src/debug.c
	gcc -c $(CFLAGS) -o $@ $<
	