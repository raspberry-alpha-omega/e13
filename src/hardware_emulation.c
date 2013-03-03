#include <stdio.h>

#include "e13.h"

// the memory model, simulating basic RAM so that I can get as close to Chuck's original design as possible.
byte bytes[MEMORY_SIZE];
byte* memory_start = bytes;

// memory-mapped devices, allowing dynamic plug/unplug
struct device device[32];

// memory access functions, with extra protection for tests
uint8_t byte_read(address p) {
  if (p < (word)bytes || p >= (word)(bytes + MEMORY_SIZE)) {
    printf("attempt to read byte outside memory map[%08x-%08x] (p=%08x)\n", bytes, bytes + MEMORY_SIZE, p);
    return 0;
  }
  return *(byte*)p;
}
void byte_write(address p, byte v) {
  if (p < (word)bytes || p >= (word)(bytes + MEMORY_SIZE)) {
    printf("attempt to write byte outside memory map[%08x-%08x] (p=%08x) (v=%08x)\n", bytes, bytes + MEMORY_SIZE, p, v);
    return;
  }
  *(byte*)p = v;
}
word word_read(address p) {
  if (p < (word)bytes || p >= (word)(bytes + MEMORY_SIZE)) {
    printf("attempt to read word outside memory map[%08x-%08x] (p=%08x)\n", bytes, bytes + MEMORY_SIZE, p);
    return 0;
  }
  word ret = 0;
  ret += ((word)byte_read(p+0));
  ret += ((word)byte_read(p+1))<<8;
#if WORDSIZE > 2
  ret += ((word)byte_read(p+2))<<16;
  ret += ((word)byte_read(p+3))<<24;
#if WORDSIZE > 4
  ret += ((word)byte_read(p+4))<<32;
  ret += ((word)byte_read(p+5))<<40;
  ret += ((word)byte_read(p+6))<<48;
  ret += ((word)byte_read(p+7))<<56;
#endif
#endif
//printf("word_read(p=%d)=>%d\n", p, ret);
  return ret;
}
void word_write(address p, word v) {
  if (p < (word)bytes || p >= (word)(bytes + MEMORY_SIZE)) {
    printf("attempt to write word outside memory map[%08x-%08x] (p=%08x) (v=%08x)\n", bytes, bytes + MEMORY_SIZE, p, v);
    return;
  }
//printf("word_write(p=%d,v=%d)\n", p, v);
  byte_write(p+0, (v & 0x00FF));
  byte_write(p+1, (v & 0xFF00) >> 8);
#if WORDSIZE > 2
  byte_write(p+2, (v & 0x00FF0000) >> 16);
  byte_write(p+3, (v & 0xFF000000) >> 24);
#if WORDSIZE > 4
  byte_write(p+4, (v & 0x000000FF00000000) >> 32);
  byte_write(p+5, (v & 0x0000FF0000000000) >> 40);
  byte_write(p+6, (v & 0x00FF000000000000) >> 48);
  byte_write(p+7, (v & 0xFF00000000000000) >> 56);
#endif
#endif
}

void hardware_init() {

}

