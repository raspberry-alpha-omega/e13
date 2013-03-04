#include "e13.h"

#if TEST
#include "debug.h"
#include <stdio.h>
#endif

// stack functions
void push(word v) {
  word_write(DS_TOP, v);
  DS_TOP += WORDSIZE;
}
word pop(void) {
  DS_TOP -= WORDSIZE;
  return word_read(DS_TOP);
}
void rpush(address p) {
  word_write(RS_TOP, p);
  RS_TOP += WORDSIZE;
}
address rpop(void) {
  RS_TOP -= WORDSIZE;
  return word_read(RS_TOP);
}

address dlup(address symbol) {
  address head = DICT_HEAD;
  while (head != 0) {
    if (word_read(head+DENT_NAME) == symbol) {
          return head;
    }
    head = word_read(head+DENT_PREV);
  }
  return NOT_FOUND;
}

word roundup(address p) {
  word mod = p % WORDSIZE;
  if (mod > 0) {
    p += WORDSIZE - mod;
  }
  return p;
}

address padd(address start, word len) {
//printf("badd start POOL_HEAD=%d POOL_NEXT=%d\n", POOL_HEAD, POOL_NEXT);
  address here = POOL_NEXT;
  for (int i = 0; i < len; ++i) {
//    uint8_t c = byte_read(start + i);
    uint8_t c = ((byte*)start)[i];
    if (0 == c) break;
    byte_write(here+PENT_DATA + i, c);
//printf("badd [%s] loop len=%d\n", real_address(start), len);
  }
  word next = here + PENT_DATA + roundup(len);

  word_write(here + PENT_LEN, len);
  word_write(here + PENT_NEXT, next);

  POOL_HEAD = here;
  POOL_NEXT = next;
//printf("badd end POOL_HEAD=%d POOL_NEXT=%d\n", POOL_HEAD, POOL_NEXT);
//dump_pent(POOL_HEAD);
  return here;
}

address plup(address start, word length) {
  address p = POOL_START;
  while (p < POOL_NEXT) {
    word len = word_read(p);
    address data = p + PENT_DATA;
    if (len == length) {
      int i;
      for (i = 0; i < length; ++i) {
        byte requested = byte_read(start+i);
        byte found = byte_read(data+i);
        if (requested != found) break;
      }
      if (i == length) {
              return p;
      }
    }
    p = word_read(p + PENT_NEXT);
  }

  return NOT_FOUND;
}

address pens(address start, word length) {
  address ret = plup(start, length);
  if (NOT_FOUND == ret) {
    ret = padd(start, length);
  }

  return ret;
}

int number(address start, int length) {
  int negative = 0;

  if ('-' == byte_read(start)) {
    negative = 1;
    ++start;
    --length;
  }

  if (0 == length) return 0;

  int n = 0;

  for (int i = 0 ;i < length; ++i) {
    byte c = byte_read(start+i);
    if (0 == c || ' ' == c) {
      if (0 == i) return 0;
      break;
    }
    if (c < '0' || c > '9') {
          return 0;
    } else {
      n *= 10;
      n += (c-'0');
    }
  }

  push(negative ? -n : n);
  return 1;
}

void eval_word(address p, int length) {
  address dent = NOT_FOUND;
  address name = plup(p, length);
  if (name != NOT_FOUND) {
    dent = dlup(name);
    if (dent != NOT_FOUND) {
      typefn fn = (typefn)word_read(dent+DENT_TYPE);
      word param = word_read(dent+DENT_PARAM);
      fn(param);
    } else {
#if TEST
      printf("unknown word [");
      for (int i = 0; i < length; ++i) putchar(byte_read(p+i));
      printf("]\n");
#endif
    }
  } else {
    number(p, length);
  }
}

void evaluate(address p, address next) {
  address start = p;
  address end = start;
  address scratchp = SCRATCH_START;

  int state = OUTSIDE;
  int depth = 0;

  address i;
  for(i = p; i < next; ++i) {
    char c = byte_read(i);
    switch(state) {
    case OUTSIDE:
      if (STRING_START == c) {
        state = INSTRING;
        scratchp = SCRATCH_START;
        ++depth;
      } else if (' ' != c) {
        start = i;
        state = INSIDE;
      }
      break;
    case INSIDE:
      if (' ' == c) {
        eval_word(start,i-start);
        state = OUTSIDE;
        start = i;
      } else {
        ++end;
      }
      break;
    case INSTRING:
      if (STRING_END == c) {
        --depth;
        if (0 == depth) {
          byte_write(scratchp, 0);
          push(pens(SCRATCH_START, scratchp-SCRATCH_START));
          state = OUTSIDE;
        } else {
          byte_write(scratchp++, c);
        }
      } else {
        if (STRING_START == c) {
          ++depth;
        }
        byte_write(scratchp++, c);
      }
      break;
    }
  }

  if (i > start) {
    eval_word(start,next-start);
  }
}

void definition(address pent) {
  address start = pent+PENT_DATA;
  word length = word_read(pent+PENT_LEN);
  evaluate(start, start + length);
}

void primitive(address p) {
  ((primfn)p)();
}

void literal(address p) {
  push(p);
}

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

void dict_offset(word offset) {
  push(DICT_NEXT + offset);
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

void dent_blank() {
  word_write(DICT_NEXT+DENT_NAME, 0);
  word_write(DICT_NEXT+DENT_TYPE, 0);
  word_write(DICT_NEXT+DENT_PARAM, 0);
  word_write(DICT_NEXT+DENT_PREV, DICT_HEAD);
}

void dent_next(void) {
  DICT_HEAD = DICT_NEXT;
  DICT_NEXT += DENT_SIZE;
  dent_blank();
}

#define DADD(name, nlen, type, param) \
  word_write(DICT_NEXT+DENT_NAME, padd((address)name, nlen)); \
  word_write(DICT_NEXT+DENT_TYPE, (address)type); \
  word_write(DICT_NEXT+DENT_PARAM, (word)param); \
  dent_next(); \

#define DEF(name, nlen, body, blen) \
  DADD(name, nlen, &definition, padd((address)body, blen))

void init_vars(void) {
  DADD("DICT_HEAD", 9, &literal, &DICT_HEAD);
  DADD("DICT_NEXT", 9, &literal, &DICT_NEXT);
  DADD("DEF_FN", 6, &literal, &definition);

  DADD("DENT_NAME", 9, &dict_offset, DENT_NAME);
  DADD("DENT_TYPE", 9, &dict_offset, DENT_TYPE);
  DADD("DENT_PARAM", 10, &dict_offset, DENT_PARAM);
  DADD("DENT_PREV", 9, &dict_offset, DENT_PREV);
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
  DEF("dent+", 5, "W+ W+ W+ W+", 11);
  DEF("dent_next", 9, "DICT_NEXT @ DICT_HEAD ! DICT_NEXT @ dent+ DICT_NEXT !", 53);
  DEF("dent_blank", 10, "DICT_HEAD @ DENT_PREV ! 0 DENT_NAME ! 0 DENT_TYPE ! 0 DENT_PARAM !", 66);
  DEF("def", 3, "dent_set dent_next dent_blank", 29);
}

// set up default entries and initialise variables
void init() {
  // set "constants"
  INBUF_START = (address)(memory_start + sizeof(struct sys_const) + sizeof(struct sys_var));
  INBUF_END = INBUF_START + INBUF_BYTES;

  DSTACK_START = INBUF_END;
  DSTACK_END = DSTACK_START + DSTACK_WORDS * WORDSIZE;

  RSTACK_START = DSTACK_END;
  RSTACK_END = RSTACK_START + RSTACK_WORDS * WORDSIZE;

  DICT_START = RSTACK_END;
  DICT_END = DICT_START + DICT_WORDS * WORDSIZE;

  SCRATCH_START = DICT_END;
  SCRATCH_END = SCRATCH_START + SCRATCH_BYTES;

  POOL_START = SCRATCH_END;
  POOL_END = POOL_START + POOL_BYTES;

  // set "variables"
  INBUF_IN = INBUF_START;
  INBUF_OUT = INBUF_START;

  DS_TOP = DSTACK_START;
  RS_TOP = RSTACK_START;

  POOL_NEXT = POOL_START;
  POOL_HEAD = POOL_START;
  padd(INBUF_START,0);

  DICT_HEAD = 0;
  DICT_NEXT = DICT_START;
  dent_blank();

  init_vars();
  init_prims();
  init_defs();
}
