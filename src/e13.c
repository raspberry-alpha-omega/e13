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
  address p = DICT_HEAD;
  while (p != 0) {
    if (word_read(p+DENT_NAME) == symbol) {
      return p;
    } else {
    }
    p = word_read(p+PENT_PREV);
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
  word_write(HEAP_NEXT + PENT_LEN, len);
  word_write(HEAP_NEXT + PENT_PREV, POOL_HEAD);

  for (int i = 0; i < len; ++i) {
    uint8_t c = ((byte*)start)[i];
    if (0 == c) break;
    byte_write(HEAP_NEXT+PENT_DATA + i, c);
  }

  POOL_HEAD = HEAP_NEXT;
  HEAP_NEXT += roundup(PENT_DATA + len);

  return POOL_HEAD;
}

address plup(address start, word length) {
  address p = POOL_HEAD;
  while (p != 0) {
    word len = word_read(p+PENT_LEN);
    if (len == length) {
      address data = p + PENT_DATA;
      int i;
      for (i = 0; i < length; ++i) {
        byte requested = byte_read(start+i);
        byte found = byte_read(data+i);
        if (requested != found) {
          break;
        }
      }
      if (i == length) {
        return p;
      }
    }
    p = word_read(p + PENT_PREV);
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

void dict_offset(word offset) {
  push(HEAP_NEXT + offset);
}

void dent_next(void) {
  DICT_HEAD = HEAP_NEXT;
  HEAP_NEXT += DENT_SIZE;
}
