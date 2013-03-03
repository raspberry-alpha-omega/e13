#include <stdio.h>

#include "e13.h"
#include "debug.h"

#define CTRLC 0x03

int main(void) {
  char buf[16384];
  INBUF_IN = INBUF_START;

  init();

  for (;;) {
    int c = getchar();
    if ('\n' == c) {
      evaluate(INBUF_START, INBUF_IN);
      INBUF_IN = INBUF_START;
      dump_stack();
    } else if (c < ' ') {
      break;
    } else {
      byte_write(INBUF_IN++, c);
      byte_write(INBUF_IN, 0);
    }
  }
}
