#include <stdio.h>

#include "e13.h"
#include "debug.h"

void dump_sysconsts() {
  printf("system consts:\n");
  printf(" inbuf_start=    %*x\n", WORDSIZE*2, INBUF_START);
  printf(" inbuf_end=      %*x\n", WORDSIZE*2, INBUF_END);
  printf(" dstack_start=   %*x\n", WORDSIZE*2, DSTACK_START);
  printf(" dstack_end=     %*x\n", WORDSIZE*2, DSTACK_END);
  printf(" rstack_start=   %*x\n", WORDSIZE*2, RSTACK_START);
  printf(" rstack_end=     %*x\n", WORDSIZE*2, RSTACK_END);
  printf(" dict_start=     %*x\n", WORDSIZE*2, DICT_START);
  printf(" dict_end=       %*x\n", WORDSIZE*2, DICT_END);
  printf(" scratch_start=  %*x\n", WORDSIZE*2, SCRATCH_START);
  printf(" scratch_end=    %*x\n", WORDSIZE*2, SCRATCH_END);
  printf(" pool_start=     %*x\n", WORDSIZE*2, POOL_START);
  printf(" pool_end=       %*x\n", WORDSIZE*2, POOL_END);
}

void dump_sysvars() {
  printf("system vars:\n");
  printf(" ds_top=         %*x\n", WORDSIZE*2, DS_TOP);
  printf(" rs_top=         %*x\n", WORDSIZE*2, RS_TOP);
  printf(" dict_head=      %*x\n", WORDSIZE*2, DICT_HEAD);
  printf(" dict_next=      %*x\n", WORDSIZE*2, DICT_NEXT);
  printf(" pool_head=      %*x\n", WORDSIZE*2, POOL_HEAD);
  printf(" pool_next=      %*x\n", WORDSIZE*2, POOL_NEXT);
  printf(" inbuf_in=       %*x\n", WORDSIZE*2, INBUF_IN);
  printf(" inbuf_out=      %*x\n", WORDSIZE*2, INBUF_OUT);
}


void dump_stack(void) {
  printf("stack[ ");
  for (int i = DSTACK_START; i < DS_TOP; i += WORDSIZE) {
    printf("%08x ", word_read(i));
  }
  printf("]\n");
}

void dump_pent_s(address i, word length) {
  address data = i + PENT_DATA;
  for (int c = 0; c < length; ++c) {
    printf("%c", byte_read(data+c));
  }
}

address dump_pent(address i) {
  word length = word_read(i + PENT_LEN);
  printf(" %08x: %d[", i, length);
  dump_pent_s(i, length);
  printf("]\n");
  return word_read(i + PENT_NEXT);
}

void dump_pent_if(address a) {
  address p = POOL_START;
  while (p < POOL_NEXT) {
    if (p == a) {
      word len = word_read(p + PENT_LEN);
      putchar('[');
      if (len > 0) dump_pent_s(p, len);
      putchar(']');
      return;
    }
    p = word_read(p + PENT_NEXT);
  }
  printf("%08x", a);
}

void dump_pool(void) {
  printf("pool (START=%08x,HEAD=%08x,NEXT=%08x,END=%08x) [\n", POOL_START, POOL_HEAD, POOL_NEXT, POOL_END);
  for (int i = POOL_START; i != POOL_NEXT; ) {
    i = dump_pent(i);
  }
  printf("]\n");
}

char namebuf[128];
const char* typename(address type) {
  void* p = (void*)type;
  if (p == defined) return "def";
  if (p == primitive) return "prim";
  if (p == literal) return "lit";
  sprintf(namebuf, "%08x", type);
  return namebuf;
}

address dump_dent(address i) {
  address prev = word_read(i+DENT_PREV);
  address name = word_read(i+DENT_NAME);
  address type = word_read(i+DENT_TYPE);
  word param = word_read(i+DENT_PARAM);

  printf(" %08x: ", i);
  dump_pent_if(name);
  printf(" = %s(", typename(type));
  dump_pent_if(param);
  printf(") -> %08x\n", prev);
  return prev;
}

void dump_dict() {
  printf("dict (START=%08x,HEAD=%08x,NEXT=%08x,END=%08x) [\n", DICT_START, DICT_HEAD, DICT_NEXT, DICT_END);
  for (int i = DICT_HEAD; i >= DICT_START; ) {
    i = dump_dent(i);
  }
  printf("]\n");
}

