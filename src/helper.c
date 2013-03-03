#include "e13.h"
#include "helper.h"

void type(const char* s) {
  INBUF_IN = INBUF_START;
  for (int i = 0; s[i] != 0; ++i) {
    byte_write(INBUF_IN++, s[i]);
  }
  byte_write(INBUF_IN, 0);
}

void evaluate_INBUF(void) {
  evaluate(INBUF_START, INBUF_IN);
}

void enter(const char* s) {
  type(s);
  evaluate_INBUF();
}
