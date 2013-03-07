#include <stdio.h>

#include "e13.h"
#include "debug.h"

void dump_sysconsts() {
  printf("system consts:\n");
  printf(" dstack_start=   %*x\n", WORDSIZE*2, DSTACK_START);
  printf(" dstack_end=     %*x\n", WORDSIZE*2, DSTACK_END);
  printf(" rstack_start=   %*x\n", WORDSIZE*2, RSTACK_START);
  printf(" rstack_end=     %*x\n", WORDSIZE*2, RSTACK_END);
  printf(" scratch_start=  %*x\n", WORDSIZE*2, SCRATCH_START);
  printf(" scratch_end=    %*x\n", WORDSIZE*2, SCRATCH_END);
  printf(" pool_start=     %*x\n", WORDSIZE*2, POOL_START);
  printf(" pool_end=       %*x\n", WORDSIZE*2, POOL_END);
  printf(" inbuf_start=    %*x\n", WORDSIZE*2, INBUF_START);
  printf(" inbuf_end=      %*x\n", WORDSIZE*2, INBUF_END);
}

void dump_sysvars() {
  printf("system vars:\n");
  printf(" ds_top=         %*x\n", WORDSIZE*2, DS_TOP);
  printf(" rs_top=         %*x\n", WORDSIZE*2, RS_TOP);
  printf(" dict_head=      %*x\n", WORDSIZE*2, DICT_HEAD);
  printf(" dict_next=      %*x\n", WORDSIZE*2, HEAP_NEXT);
  printf(" pool_head=      %*x\n", WORDSIZE*2, POOL_HEAD);
  printf(" pool_next=      %*x\n", WORDSIZE*2, HEAP_NEXT);
  printf(" inbuf_in=       %*x\n", WORDSIZE*2, INBUF_IN);
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
  return word_read(i + PENT_PREV);
}

void dump_pent_if(address a) {
  address p = POOL_HEAD;
  while (p != 0) {
    if (p == a) {
      word len = word_read(p + PENT_LEN);
      putchar('[');
      if (len > 0) dump_pent_s(p, len);
      putchar(']');
      return;
    }
    p = word_read(p + PENT_PREV);
  }
  printf("%08x", a);
}

void dump_pool(void) {
  printf("pool (HEAD=%08x,NEXT=%08x) [\n", POOL_HEAD, HEAP_NEXT);
  for (int i = POOL_HEAD; i != 0; ) {
    i = dump_pent(i);
  }
  printf("]\n");
}

char namebuf[128];
const char* typename(address type) {
  void* p = (void*)type;
  if (p == definition) return "def";
  if (p == primitive) return "prim";
  if (p == literal) return "lit";
  if (p == dict_offset) return "dict";
  sprintf(namebuf, "%08x", type);
  return namebuf;
}

address dump_dent(address i) {
  address len = word_read(i+PENT_LEN);
  address prev = word_read(i+PENT_PREV);
  address name = word_read(i+DENT_NAME);
  address type = word_read(i+DENT_TYPE);
  word param = word_read(i+DENT_PARAM);

  printf(" %08x: name[", i);
  dump_pent_if(name);
  printf("] = %s(", typename(type));
  dump_pent_if(param);
  printf(") len=%d -> %08x\n", len, prev);
  return prev;
}

void dump_dict() {
  printf("dict (HEAD=%08x,NEXT=%08x) [\n", DICT_HEAD, HEAP_NEXT);
  for (int i = DICT_HEAD; i != 0; ) {
    i = dump_dent(i);
  }
  printf("]\n");
}

void dump_stack(void) {
  printf("stack[ ");
  for (int i = DSTACK_START; i < DS_TOP; i += WORDSIZE) {
    word v = word_read(i);
    dump_pent_if(v);
    putchar(' ');
  }
  printf("]\n");
}
