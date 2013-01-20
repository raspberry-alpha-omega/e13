#include <stdio.h> // debugging only, remove for final version

#include <stdint.h>
#include "io.h"
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
uint32_t dict_read(address p) {
  return dict[p];
}
void dict_write(address p, word v) {
  dict[p] = v;
}

void dadd(void) {
  address old_next = DICT_NEXT;
  DICT_HEAD = old_next;

  DICT_NEXT += 4;
  dict[DICT_NEXT+DENT_NAME] = 0;
  dict[DICT_NEXT+DENT_TYPE] = 0;
  dict[DICT_NEXT+DENT_PARAM] = 0;
  dict[DICT_NEXT+DENT_PREV] = old_next;
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

word roundup(word p) {
  word mod = p % 4;
  if (mod > 0) {
    p += 4 - mod;
  }
  return p;
}

address blookup(address start, int length) {
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

    p += PENT_DATA; // step past length word
    p += roundup(len); // and the characters in the string
  }

  return 0xFFFFFFFF;
}

address badd(address start, int len) {
  address old_next = POOL_NEXT;
  word_write(POOL_NEXT, len);
  POOL_NEXT += PENT_DATA;
  POOL_NEXT += roundup(len);
  POOL_HEAD = old_next;

  for (int i = 0; i < len; ++i) {
    byte_write(POOL_HEAD+PENT_DATA + i, byte_read(start + i));
  }
  return POOL_HEAD;
}

void execute(address p) {
  // TODO
}

void eval(address p, int length) {
  // TODO
}

int number_fn(address dent, address start, int len) {
  int n = 0;

  for (int i = 0; i < len; ++i) {
    byte c = byte_read(start+i);
    if (c < '0' || c > '9') {
      return 0;
    } else {
      n *= 10;
      n += (c-'0');
    }
  }
  push(n);
  return 1;
}


// system variables, really these belong in target memory, perhaps before DSTACK
address DS_TOP = DSTACK_START;
address RS_TOP = RSTACK_START;
address DICT_HEAD = DICT_START;
address DICT_NEXT = DICT_START+4;
address POOL_HEAD = POOL_START;
address POOL_NEXT = POOL_START+4;
address RING_IN = INRING_START;
address RING_OUT = INRING_START;
word INPUT_COUNT = 0;

// the memory model, simulating basic RAM so that I can get as close to Chuck's original design as possible.
word dstack[DSTACK_WORDS];
word rstack[RSTACK_WORDS];
word dict[DICT_WORDS] = {
    0, (word)&number_fn, 0, 0,
    0, 0, 0, DICT_START
};
byte bytes[POOL_BYTES + INRING_BYTES] = {
    0,0,0,0,
};

void run(void) {
  // TODO
}

#if 0
int match(int defname, int start, int end) {
  if (0 == defname) return 1; // NULL is a wildcard which always matches

  int len = end - start;
  const char* s = (const char*)defname;

  for (int i = 0; i < len; ++i) {
    if (s[i] == 0) return 0;
    if (s[i] != GETBYTE(INBUFFER+start+i)) return 0;
  }
  return s[len] == 0;
}

void eval(int buf, int start, int end) {
  int walk = mem[HEAD];
  do {
    if (match(mem[walk], start, end)) {
      typefn f = (typefn)mem[walk+1];
      if (f(walk, buf, start, end)) return;
    }
    walk = mem[walk+3];
  } while (walk != 0);

  console_write_string("NOTFOUND");
}

void exec(int buf, int count) {
  int start = 0;
  int end = 0;
  int state = OUTSIDE;
  int i = 0;

  while (i < count) {
    char c = GETBYTE(buf+i);
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
        eval(buf, start, i);
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
    eval(buf, start,i);
  }
}

void input(void) {
  exec(INBUFFER, mem[INCOUNT]);
  mem[INCOUNT] = 0;
}

void nop_fn(int dent, int buf, int start, int end) {}

int fn_fn(int dent, int buf, int start, int end) {
  fn f = (fn)mem[dent+2];
  f(start,end);
  return 1;
}

int prints_fn(int dent, int buf, int start, int end) {
  console_write_string((const char*)mem[dent+2]);
  return 1;
}

void eol(int start, int end) {
  console_write_char('\n');
}

int number_fn(int dent, int buf, int start, int end) {
  int n = 0;

  for (int i = start; i < end; ++i) {
    char c = GETBYTE(buf+i);
    if (c < '0' || c > '9') {
      return 0;
    } else {
      n *= 10;
      n += (c-'0');
    }
  }
  console_write_number(n);
  console_write_char(' ');
  return 1;
}

int exec_fn(int dent, int buf, int start, int end) {
printf("exec_fn, dent=%d, buf=%d, start=%d, end=%d\n", dent, buf, start,end);
dump("exec_fn");
  exec(mem[dent+2], 3);
  return 1;
}

void init(void) {
  int p = DICT;

  mem[p++] = 0;
  mem[p++] = (int)number_fn;
  mem[p++] = 0;
  mem[p++] = 0;
  mem[HEAD] = DICT;

  mem[p++] = (int)".";
  mem[p++] = (int)fn_fn;
  mem[p++] = (int)&eol;
  mem[p++] = mem[HEAD];
  mem[HEAD] += 4;
}

void dadd(const char* name, typefn type, int data) {
  int dp = mem[HEAD] + 4;
  mem[dp] = (int)name;
  mem[dp+1] = (int)type;
  mem[dp+2] = data;
  mem[dp+3] = mem[HEAD];
  mem[HEAD] += 4;
}

void type(const char* s) {
  int i = 0;
  while (*s) {
    mem[INBUFFER + i++] = *s++;
  }

  mem[INCOUNT] = i;
}
#endif
