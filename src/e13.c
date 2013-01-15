#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define GETBYTE(i) (mem[i] & 0xFF)

int mem[65536] = {
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0
    // ...
};

enum label {
  HEAD,
  INCOUNT,
  INBUFFER,
  DICT = INBUFFER + 30,
};

#define COLS 4
void dump(const char* s) {
  puts(s);
  for (int i = 0; i < (64/COLS); ++i) {
    int row = i * COLS;
    for (int j = 0; j < COLS; ++j) {
      printf("%08x ", mem[row + j]);
    }
    printf("\n");
  }
  printf("HEAD=%d\n", mem[HEAD]);
  printf("INCOUNT=%d\n", mem[INCOUNT]);

  printf("INBUFFER[");
  for (int i = 0; i < mem[INCOUNT]; ++i) {
    printf("%c", GETBYTE(INBUFFER+i));
  }
  printf("]\nDICT:\n");
  int end = (mem[HEAD] < 60) ? mem[HEAD]+4 : 64;
  for (int i = DICT; i < end; i += 4) {
    printf("  NAME=%08x TYPE=%08x DATA=%08x PREV=%8d\n", mem[i], mem[i+1], mem[i+2], mem[i+3]);
  }
  puts("");
}

#define OUTSIDE 0
#define INSIDE 1

typedef void (*fn)(int start, int end);
typedef int (*typefn)(int dent, int start, int end); // return 1 if handled, 0 otherwise

void nop_fn(int dent, int start, int end) {}

int fn_fn(int dent, int start, int end) {
  fn f = (fn)mem[dent+2];
  f(start,end);
  return 1;
}

int prints_fn(int dent, int start, int end) {
  printf("%s ", mem[dent+2]);
  return 1;
}

void eol(int start, int end) {
  putchar('\n');
}

int number_fn(int dent, int start, int end) {
  int n = 0;

  for (int i = start; i < end; ++i) {
    char c = GETBYTE(INBUFFER+i);
    if (!isdigit(c)) {
      return 0;
    } else {
      n *= 10;
      n += (c-'0');
    }
  }
  printf("%d ", n);
  return 1;
}

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

void eval(int start, int end) {
  int walk = mem[HEAD];
  do {
    if (match(mem[walk], start, end)) {
      typefn f = (typefn)mem[walk+1];
      if (f(walk, start, end)) return;
    }
    walk = mem[walk+3];
  } while (walk != 0);

  printf("NOTFOUND");
}

void input(void) {
  int start = 0;
  int end = 0;
  int state = OUTSIDE;
  int i = 0;
  int count = mem[INCOUNT];

  while (i < count) {
    char c = GETBYTE(INBUFFER+i);
    switch(state) {
    case OUTSIDE:
      if (' ' != c) {
        end = start = i;
        state = INSIDE;
      }
      break;
    case INSIDE:
      if (' ' == c) {
        eval(start, i);
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
    eval(start,i);
  }

  mem[INCOUNT] = 0;
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

	dadd("skidoo", prints_fn, (int)"honey");

	type("23 skidoo .");
	input();

	return 0;
}
