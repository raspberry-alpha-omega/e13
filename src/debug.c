#include <stdio.h>

#include "e13.h"
#include "debug.h"

void dump_sysconsts() {
  printf("system consts:\n");
  printf(" inbuf_start=    %*x\n", WORDSIZE*2, sys_consts->inbuf_start);
  printf(" inbuf_end=      %*x\n", WORDSIZE*2, sys_consts->inbuf_end);
  printf(" dstack_start=   %*x\n", WORDSIZE*2, sys_consts->dstack_start);
  printf(" dstack_end=     %*x\n", WORDSIZE*2, sys_consts->dstack_end);
  printf(" rstack_start=   %*x\n", WORDSIZE*2, sys_consts->rstack_start);
  printf(" rstack_end=     %*x\n", WORDSIZE*2, sys_consts->rstack_end);
  printf(" dict_start=     %*x\n", WORDSIZE*2, sys_consts->dict_start);
  printf(" dict_end=       %*x\n", WORDSIZE*2, sys_consts->dict_end);
  printf(" scratch_start=  %*x\n", WORDSIZE*2, sys_consts->scratch_start);
  printf(" scratch_end=    %*x\n", WORDSIZE*2, sys_consts->scratch_end);
  printf(" pool_start=     %*x\n", WORDSIZE*2, sys_consts->pool_start);
  printf(" pool_end=       %*x\n", WORDSIZE*2, sys_consts->pool_end);
}

void dump_sysvars() {
  printf("system vars:\n");
  printf(" ds_top=         %*x\n", WORDSIZE*2, sys_vars->ds_top);
  printf(" rs_top=         %*x\n", WORDSIZE*2, sys_vars->rs_top);
  printf(" dict_head=      %*x\n", WORDSIZE*2, sys_vars->dict_head);
  printf(" dict_next=      %*x\n", WORDSIZE*2, sys_vars->dict_next);
  printf(" pool_head=      %*x\n", WORDSIZE*2, sys_vars->pool_head);
  printf(" pool_next=      %*x\n", WORDSIZE*2, sys_vars->pool_next);
  printf(" inbuf_in=       %*x\n", WORDSIZE*2, sys_vars->inbuf_in);
  printf(" inbuf_out=      %*x\n", WORDSIZE*2, sys_vars->inbuf_out);
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

