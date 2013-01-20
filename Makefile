CFLAGS=-std=c99

test: obj/test.o obj/e13.o obj/io.o obj/debug.o
	gcc -o test obj/test.o obj/e13.o obj/io.o obj/debug.o
	
e13: obj/e13.o obj/io.o obj/debug.o obj/main.o
	gcc -o e13 obj/main.o obj/e13.o obj/io.o obj/debug.o

obj/%.o: src/%.c
	gcc -c $(CFLAGS) -o $@ $<
	
clean:
	rm -f obj/*.o
	