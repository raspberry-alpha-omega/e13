CFLAGS=-std=c99

test: obj/test.o obj/e13.o obj/debug.o
	gcc -o $@ $^
	./test
	
e13: obj/e13.o obj/main.o
	gcc -o $@ $^

obj/%.o: src/%.c
	gcc -c $(CFLAGS) -o $@ $<
	
clean:
	rm -f obj/*.o
	