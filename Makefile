CFLAGS= -std=c99

all: clean test e13_generic

test: src/test.c src/e13.c src/debug.c src/hardware_emulation.c
	gcc $(CFLAGS) -DTEST -m32 -o $@ $^
	./test

e13_generic: src/main.c src/e13.c src/hardware_emulation.c src/debug.c
	gcc $(CFLAGS) -m32 -o $@ $^

e13_raspi: src/start_raspi.s src/main.c src/e13.c src/mem_raspi.c
	gcc $(CFLAGS) -m32 -static -nostdlib -ffreestanding -o $@ $^

clean:
	rm -f obj/*.o
	rm -f e13* test*



# deprecated targets

all64: all test_64 e13_x86_64

test_64: src/test.c src/e13.c src/debug.c
	gcc $(CFLAGS) -DTEST -m64 -o $@ $^
	./test_64

e13_x86: src/start_x86.s src/main.c src/e13.c src/mem.c src/hardware_emulation.c
	gcc $(CFLAGS) -m32 -static -nostdlib -ffreestanding -o $@ $^

e13_x86_64: src/start_x86_64.s src/main.c src/mem.c src/e13.c
	gcc $(CFLAGS) -m64 -static -nostdlib -ffreestanding -o $@ $^
