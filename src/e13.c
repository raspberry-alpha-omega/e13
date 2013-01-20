#include <stdio.h>

#include "e13.h"
#include "io.h"

// the memory model, simulating basic RAM so that I can get as close to Chuck's original design as possible.
int mem[65536] = {
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
    // ...
};

#define OUTSIDE 0
#define INSIDE 1

typedef void (*fn)(int start, int end);
typedef int (*typefn)(int dent, int buf, int start, int end); // return 1 if handled, 0 otherwise

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

int main(void) {
  init();

	type("23 skidoo .");
	input();

  dadd("xx", exec_fn, (int)"23 skidoo .");
  type("13 xx");
  input();

	dadd("skidoo", prints_fn, (int)"honey");

	type("23 skidoo .");
	input();

	return 0;
}
