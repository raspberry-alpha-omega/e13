#include <stdio.h>

#include "e13.h"

#define COLS 4
void dump(const char* label) {
  puts(label);
//  for (int i = 0; i < (END/COLS); ++i) {
//    int row = i * COLS;
//    for (int j = 0; j < COLS; ++j) {
//      printf("%08x ", mem[row + j]);
//    }
//    printf("\n");
//  }
//  printf("HEAD=%d\n", mem[HEAD]);
//  printf("INCOUNT=%d\n", mem[INCOUNT]);
//
//  printf("INBUFFER[");
//  for (int i = 0; i < mem[INCOUNT]; ++i) {
//    printf("%c", GETBYTE(INBUFFER+i));
//  }
//  printf("]\nDICT:\n");
//  int end = (mem[HEAD] < 60) ? mem[HEAD]+4 : 64;
//  for (int i = DICT; i < end; i += 4) {
//    printf("  %4x NAME=%08x(%s) TYPE=%08x DATA=%08x PREV=%4x\n", i, mem[i], (void*)mem[i], mem[i+1], mem[i+2], mem[i+3]);
//  }
//  puts("");
}
