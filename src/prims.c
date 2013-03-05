#include "e13.h"
#include "prims.h"

void prim_b_plus() {
  push(pop() + (word)1);
}

void prim_w_plus() {
  push(pop() + WORDSIZE);
}

void prim_b_read() {
  push(byte_read(pop()));
}

void prim_b_write() {
  address p = pop();
  word v = pop();
  byte_write(p, (byte)v);
}

void prim_w_read() {
  push(word_read(pop()));
}

void prim_w_write() {
  address p = pop();
  word v = pop();
  word_write(p, v);
}

void prim_dup(void) {
  word x = pop();
  push(x);
  push(x);
}

void prim_drop(void) {
  pop();
}

void prim_each_c(void) {
  address code = pop();
  address string = pop();
  int len = word_read(string + PENT_LEN);
  for (int i = 0; i < len; ++i) {
    push(byte_read(string+PENT_DATA+i));
    definition(code);
  }
}
