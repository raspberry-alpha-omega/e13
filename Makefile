CFLAGS= -std=c99

all: clean test_32 e13_x86

all64: all test_64 e13_x86_64

test_32: src/test.c src/e13.c src/debug.c
	gcc $(CFLAGS) -m32 -o $@ $^
	./test_32
	
e13_x86: src/start_x86.s src/main.c src/e13.c
	gcc $(CFLAGS) -m32 -static -nostdlib -ffreestanding -o $@ $^

test_64: src/test.c src/e13.c src/debug.c
	gcc $(CFLAGS) -m64 -o $@ $^
	./test_64
	
e13_x86_64: src/start_x86_64.s src/main.c src/e13.c
	gcc $(CFLAGS) -m64 -static -nostdlib -ffreestanding -o $@ $^
	
clean:
	rm -f obj/*.o
	rm -f e13* test*
	