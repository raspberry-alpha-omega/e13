#include <stdio.h> // debugging only, remove for final version

#include <stdint.h>
#include "e13.h"

uint8_t byte_read(address p) {
  return bytes[p];
}
void byte_write(address p, byte v) {
  bytes[p] = v;
}
word word_read(address p) {
  word ret = byte_read(p);
  ret += ((word)byte_read(p+1))<<8;
  ret += ((word)byte_read(p+2))<<16;
  ret += ((word)byte_read(p+3))<<24;
//printf("word_read(p=%d)=>%d\n", p, ret);
  return ret;
}
void word_write(address p, word v) {
//printf("word_write(p=%d,v=%d)\n", p, v);
  byte_write(p, v & 0x000000FF);
  byte_write(p+1, (v & 0x0000FF00) >> 8);
  byte_write(p+2, (v & 0x00FF0000) >> 16);
  byte_write(p+3, (v & 0xFF000000) >> 24);
}

// memory access functions
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

address dlookup(address symbol) {
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

int buflen(address buf) {
  int ret = 0;
  while (byte_read(buf+ret) != 0)
    ++ret;
  return ret;
}

address blookup(address start, word length) {
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

void primitive(address p) {
  (*((primfn*)bytes+p))();
}

void eval_word(address p, int length) {
  address dent = NOT_FOUND;
  address name = blookup(p, length);
  if (name != NOT_FOUND) {
    dent = dlookup(name);
    if (dent != NOT_FOUND) {
      typefn fn = (typefn)word_read(dent+DENT_TYPE);
      word param = word_read(dent+DENT_PARAM);
      fn(param);
    }
  } else {
    number(p);
  }
}

void evaluate(address p, address next) {
  address start = p;
  address end = start;
  address scratchp = 0;

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
      } else {
        ++end;
      }
      break;
    case INSTRING:
      if (STRING_END == c) {
        --depth;
        if (0 == depth) {
          byte_write(scratchp, 0);
          push(badd(SCRATCH_START));
          state = OUTSIDE;
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

void evaluate_pent(address pent) {
  address start = pent+PENT_DATA;
  evaluate(start, start + word_read(pent));
}

// system variables, really these belong in target memory, perhaps before DSTACK or after the byte pool
address INBUF_IN = INBUF_START;
address INBUF_OUT = INBUF_START;
word INPUT_COUNT = 0;

address DS_TOP = DSTACK_START;

address RS_TOP = RSTACK_START;

address DICT_HEAD = DICT_START;
address DICT_NEXT = DICT_START+DENT_SIZE;

address POOL_HEAD = POOL_START;
address POOL_NEXT = POOL_START+PENT_DATA;

// the memory model, simulating basic RAM so that I can get as close to Chuck's original design as possible.
byte bytes[INBUF_BYTES + (DSTACK_WORDS * WORDSIZE) + POOL_BYTES];

// set up default entries and initialise variables
void init() {
  INBUF_IN = INBUF_START;
  INBUF_OUT = INBUF_START;
  word INPUT_COUNT = 0;

  DS_TOP = DSTACK_START;

  POOL_HEAD = POOL_START;
  word_write(POOL_HEAD + PENT_LEN, 0);
  word_write(POOL_HEAD + PENT_NEXT, POOL_HEAD + PENT_DATA);
  POOL_NEXT = POOL_HEAD + PENT_DATA;

  DICT_HEAD = DICT_START;
  word_write(DICT_HEAD+DENT_NAME, POOL_START);
  word_write(DICT_HEAD+DENT_TYPE, (word)&number);
  word_write(DICT_HEAD+DENT_PARAM, 0);
  word_write(DICT_HEAD+DENT_PREV, 0);

  DICT_NEXT = DICT_START+DENT_SIZE;
  word_write(DICT_NEXT+DENT_NAME, 0);
  word_write(DICT_NEXT+DENT_TYPE, 0);
  word_write(DICT_NEXT+DENT_PARAM, 0);
  word_write(DICT_NEXT+DENT_PREV, DICT_START);

  RS_TOP = RSTACK_START;
}

void run(void) {
  // TODO
}

address badd(address start) {
//printf("badd start POOL_HEAD=%d POOL_NEXT=%d\n", POOL_HEAD, POOL_NEXT);
  address here = POOL_NEXT;
  int len = 0;
  for (;; ++len) {
    uint8_t c = byte_read(start + len);
    if (0 == c) break;
    byte_write(here+PENT_DATA + len, c);
//printf("badd [%s] loop len=%d\n", bytes+start, len);
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

void dadd(void) {
  address old_next = DICT_NEXT;
  DICT_HEAD = old_next;

  DICT_NEXT += DENT_SIZE;
  word_write(DICT_NEXT+DENT_NAME, 0);
  word_write(DICT_NEXT+DENT_TYPE, 0);
  word_write(DICT_NEXT+DENT_PARAM, 0);
  word_write(DICT_NEXT+DENT_PREV, old_next);
}

int natural(int negative, address start) {
  int n = 0;

  for (int i = 0 ;; ++i) {
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

int number(address start) {
  int negative = 0;

  if ('-' == byte_read(start)) {
    negative = 1;
    ++start;
  }

  return natural(negative, start);
}
