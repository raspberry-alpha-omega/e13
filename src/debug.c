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
    printf("%d ", word_read(i));
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
  printf(" %d: %d[", i, length);
  dump_pent_s(i, length);
  printf("]\n");
  return word_read(i + PENT_NEXT);
}

void dump_pool(void) {
  printf("pool[\n");
  for (int i = POOL_START; i != POOL_NEXT; ) {
    i = dump_pent(i);
  }
  printf("]\n");
}

address dump_dent(address i) {
  word param = word_read(i+DENT_PARAM);
  address prev = word_read(i+DENT_PREV);
  printf(" %d: %d[%s] = %p(%d", i, word_read(i+DENT_NAME), (byte*)(word_read(i+DENT_NAME)+PENT_DATA), word_read(i+DENT_TYPE), param);
  if (param >= POOL_START && param < POOL_NEXT) {
    word length = word_read(param + PENT_LEN);
    printf("[");
    dump_pent_s(param, length);
    printf("]");
  }
  printf(") %d\n", prev);
  return prev;
}

void dump_dict() {
  printf("dict[\n");
  for (int i = DICT_HEAD; i >= DICT_START; ) {
    i = dump_dent(i);
  }
  printf("]\n");
}

