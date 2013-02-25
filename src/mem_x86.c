#include "e13.h"

// memory access functions
uint8_t byte_read(address p) {
  return *(byte*)p;
}
void byte_write(address p, byte v) {
  *(byte*)p = v;
}
word word_read(address p) {
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
