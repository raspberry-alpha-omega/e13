#include "e13.h"
#include "prims.h"

#define DADD(name, nlen, type, param) {\
  address symbol = padd((address)name, nlen); \
  word_write(HEAP_NEXT+PENT_LEN, DENT_SIZE); \
  word_write(HEAP_NEXT+PENT_PREV, DICT_HEAD); \
  word_write(HEAP_NEXT+DENT_NAME, symbol); \
  word_write(HEAP_NEXT+DENT_TYPE, (address)type); \
  word_write(HEAP_NEXT+DENT_PARAM, (word)param); \
  DICT_HEAD = HEAP_NEXT; \
  HEAP_NEXT += DENT_SIZE; }

#define DEF(name, nlen, body, blen) {\
  address p = padd((address)body, blen); \
  DADD(name, nlen, &definition, p); }

void init_vars(void) {
  DADD("DICT_HEAD", 9, &literal, &DICT_HEAD);
  DADD("HEAP_NEXT", 9, &literal, &HEAP_NEXT);
  DADD("DEF_FN", 6, &literal, &definition);

  DADD("DENT_NAME", 9, &dict_offset, DENT_NAME);
  DADD("DENT_TYPE", 9, &dict_offset, DENT_TYPE);
  DADD("DENT_PARAM", 10, &dict_offset, DENT_PARAM);
  DADD("PENT_PREV", 9, &dict_offset, PENT_PREV);
}

void init_prims(void) {
  DADD("@", 1, &primitive, &prim_w_read)
  DADD("!", 1, &primitive, &prim_w_write)
  DADD("W+", 2, &primitive, &prim_w_plus)
  DADD("B@", 2, &primitive, &prim_b_read)
  DADD("B!", 2, &primitive, &prim_b_write)
  DADD("dup", 3, &primitive, &prim_dup)
  DADD("drop", 4, &primitive, &prim_drop)
  DADD("eachc", 5, &primitive, &prim_each_c);
}

void init_defs(void) {
  DEF("dent_set", 8, "DEF_FN DENT_TYPE ! DENT_NAME ! DENT_PARAM !", 43);
  DEF("dent+", 5, "W+ W+ W+ W+ W+", 11);
  DEF("dent_next", 9, "HEAP_NEXT @ DICT_HEAD ! HEAP_NEXT @ dent+ DICT_NEXT !", 53);
  DEF("dent_blank", 10, "HEAP_NEXT @ PENT_PREV ! 0 DENT_NAME ! 0 DENT_TYPE ! 0 DENT_PARAM !", 66);
  DEF("def", 3, "dent_set dent_next dent_blank", 29);
}

void reset_working_data() {
  INBUF_IN = INBUF_START;
  DS_TOP = DSTACK_START;
  RS_TOP = RSTACK_START;
}

// set up default entries and initialise variables
void corn_init() {
  // set "constants"
  INBUF_START = (address)(memory_start + sizeof(struct sys_const) + sizeof(struct sys_var));
  INBUF_END = INBUF_START + INBUF_BYTES;

  DSTACK_START = INBUF_END;
  DSTACK_END = DSTACK_START + DSTACK_WORDS * WORDSIZE;

  RSTACK_START = DSTACK_END;
  RSTACK_END = RSTACK_START + RSTACK_WORDS * WORDSIZE;

  SCRATCH_START = RSTACK_END;
  SCRATCH_END = SCRATCH_START + SCRATCH_BYTES;

  POOL_START = SCRATCH_END;
  POOL_END = POOL_START + POOL_BYTES;

  // set "variables"

  HEAP_NEXT = POOL_START;
  POOL_HEAD = 0;
  DICT_HEAD = 0;

  // Add dummy entry to heap
  padd(INBUF_START,0);

  reset_working_data();

  init_vars();
  init_prims();
  init_defs();

  reset_working_data();
}
