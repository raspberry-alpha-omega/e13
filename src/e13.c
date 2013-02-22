#include <stdio.h> // debugging only, remove for final version

#include <stdint.h>
#include "e13.h"

#include "debug.h"

// the memory model, simulating basic RAM so that I can get as close to Chuck's original design as possible.
byte bytes[MEMORY_SIZE];

byte* memory_start = bytes;
struct sys_const* sys_consts = (struct sys_const*)bytes;
struct sys_var* sys_vars = (struct sys_var*)bytes + sizeof(struct sys_const);
byte* real_address(address a) {
  return (byte*)a;
}


uint8_t byte_read(address p) {
  return *real_address(p);
}
void byte_write(address p, byte v) {
  *real_address(p) = v;
}
word word_read(address p) {
  word ret = 0;
  ret += ((word)byte_read(p+0));
  ret += ((word)byte_read(p+1))<<8;
#if WORDSIZE > 2
  ret += ((word)byte_read(p+2))<<16;
  ret += ((word)byte_read(p+3))<<24;
#if WORDSIZE > 4
  ret += ((word)byte_read(p+4))<<32;
  ret += ((word)byte_read(p+5))<<40;
  ret += ((word)byte_read(p+6))<<48;
  ret += ((word)byte_read(p+7))<<56;
#endif
#endif
//printf("word_read(p=%d)=>%d\n", p, ret);
  return ret;
}
void word_write(address p, word v) {
//printf("word_write(p=%d,v=%d)\n", p, v);
  byte_write(p+0, (v & 0x00FF));
  byte_write(p+1, (v & 0xFF00) >> 8);
#if WORDSIZE > 2
  byte_write(p+2, (v & 0x00FF0000) >> 16);
  byte_write(p+3, (v & 0xFF000000) >> 24);
#if WORDSIZE > 4
  byte_write(p+4, (v & 0x000000FF00000000) >> 32);
  byte_write(p+5, (v & 0x0000FF0000000000) >> 40);
  byte_write(p+6, (v & 0x00FF000000000000) >> 48);
  byte_write(p+7, (v & 0xFF00000000000000) >> 56);
#endif
#endif
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
  (*((primfn*)real_address(p)))();
}

int number(address start) {
  int negative = 0;

  if ('-' == byte_read(start)) {
    negative = 1;
    ++start;
  }

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

address badd(address start) {
//printf("badd start POOL_HEAD=%d POOL_NEXT=%d\n", POOL_HEAD, POOL_NEXT);
  address here = POOL_NEXT;
  int len = 0;
  for (;; ++len) {
    uint8_t c = byte_read(start + len);
    if (0 == c) break;
    byte_write(here+PENT_DATA + len, c);
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
