CFLAGS=-std=c99

test: obj/test.o obj/e13.o
	gcc -o test obj/test.o obj/e13.o
	
e13: obj/e13.o obj/main.o
	gcc -o e13 obj/main.o obj/e13.o

obj/%.o: src/%.c
	gcc -c $(CFLAGS) -o $@ $<
	
clean:
	rm -f obj/*.o
	