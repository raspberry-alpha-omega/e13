#include <stdio.h>

#include "e13.h"
#include "debug.h"
#include "helper.h"

#define CTRLC 0x03

int main(void) {
  char buf[16384];
  INBUF_IN = INBUF_START;

  hardware_init();
  init();

  enter("[539054084 B!] [cput] def");
  enter("[[cput] eachc] [print] def");
  enter("[print 10 cput] [println] def");

  INBUF_IN = INBUF_START;
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
