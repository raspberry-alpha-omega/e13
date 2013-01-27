#include <stdio.h> // debugging only, remove for final version

#include <stdint.h>
#include "e13.h"

// memory access functions
void push(word v) {
  dstack[DS_TOP++] = v;
}
word pop(void) {
  return dstack[--DS_TOP];
}
void rpush(address p) {
  rstack[RS_TOP++] = p;
}
address rpop(void) {
  return rstack[--RS_TOP];
}

address dlookup(address symbol) {
  address head = DICT_HEAD;
  while (head != 0) {
    if (dict[head+DENT_NAME] == symbol) {
      return head;
    }
    head = dict[head+DENT_PREV];
  }
  return 0xFFFFFFFF;
}

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
  return ret;
}
void word_write(address p, word v) {
  byte_write(p, v & 0x000000FF);
  byte_write(p+1, (v & 0x0000FF00) >> 8);
  byte_write(p+2, (v & 0x00FF0000) >> 16);
  byte_write(p+3, (v & 0xFF000000) >> 24);
}
uint32_t dict_read(address p) {
  return dict[p];
}
void dict_write(address p, word v) {
  dict[p] = v;
}

word roundup(word p) {
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

address blookup(address start) {
  address p = POOL_START;
  int length = buflen(start);
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

    p += PENT_DATA; // step past length word
    p += roundup(len); // and the characters in the string
  }

  return 0xFFFFFFFF;
}

void primitive(address p) {
  (*((primfn*)bytes+p))();
}

void eval_word(address p) {
  address dent = NOT_FOUND;
  address name = blookup(p);
  if (name != NOT_FOUND) {
    dent = dlookup(name);
  }
  if (dent != NOT_FOUND) {
    typefn fn = (typefn)dict_read(dent+DENT_TYPE);
    word param = dict_read(dent+DENT_PARAM);
    fn(param);
  } else {
    number(p);
  }
}

void evaluate(address p) {
  int start = 0;
  int end = 0;
  int state = OUTSIDE;
  int i = 0;

  for(;;) {
    char c = byte_read(p+i);
    if (0 == c) break;
    switch(state) {
    case OUTSIDE:
      if (' ' != c) {
        end = start = i;
        state = INSIDE;
      }
      break;
    case INSIDE:
      if (' ' == c) {
        eval_word(p+start);
        end = start = i;
        state = OUTSIDE;
      } else {
        ++end;
      }
      break;
    }
    ++i;
  }

  if (i > start) {
    eval_word(p+start);
  }
}

// system variables, really these belong in target memory, perhaps before DSTACK or after the byte pool
address DS_TOP = DSTACK_START;
address RS_TOP = RSTACK_START;
address DICT_HEAD = DICT_START;
address DICT_NEXT = DICT_START+DENT_SIZE;
address POOL_HEAD = POOL_START;
address POOL_NEXT = POOL_START+PENT_DATA;
address RING_IN = INRING_START;
address RING_OUT = INRING_START;
word INPUT_COUNT = 0;

// the memory model, simulating basic RAM so that I can get as close to Chuck's original design as possible.
word dstack[DSTACK_WORDS];
word rstack[RSTACK_WORDS];
word dict[DICT_WORDS] = {
    0, (word)&number, 0, 0,
    0, 0, 0, DICT_START
};
byte bytes[POOL_BYTES + INRING_BYTES] = {
    0,0,0,0, // entry zero has zero length
};

void run(void) {
  // TODO
}


address badd(address start) {
  address old_next = POOL_NEXT;
  int len = 0;
  for (int i = 0 ;; ++i) {
    uint8_t c = byte_read(start + i);
    if (0 == c) break;
    byte_write(POOL_NEXT+PENT_DATA + i, c);
    ++len;
  }
  word_write(POOL_NEXT, len);
  POOL_NEXT += PENT_DATA + roundup(len);
  POOL_HEAD = old_next;
  return POOL_HEAD;
}

void dadd(void) {
  address old_next = DICT_NEXT;
  DICT_HEAD = old_next;

  DICT_NEXT += DENT_SIZE;
  dict[DICT_NEXT+DENT_NAME] = 0;
  dict[DICT_NEXT+DENT_TYPE] = 0;
  dict[DICT_NEXT+DENT_PARAM] = 0;
  dict[DICT_NEXT+DENT_PREV] = old_next;
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


