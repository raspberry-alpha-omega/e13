#include <stdio.h>
#include <string.h>
#include <ctype.h>

struct Dent;

typedef void (*fn)(const char* buf, int start, int end);
typedef int (*typefn)(struct Dent* dent, const char* buf, int start, int end); // return 1 if handled, 0 otherwise

struct Dent {
	const char* name;
	typefn type;
	union { void* p; int v; } value;
	struct Dent* prev;
};

void nop_fn(struct Dent* dent, const char* buf, int start, int end) {}

int fn_fn(struct Dent* dent, const char* buf, int start, int end) {
	((fn)dent->value.p)(buf, start,end);
	return 1;
}

int prints_fn(struct Dent* dent, const char* buf, int start, int end) {
	printf("%s\n", dent->value.p);
	return 1;
}

void hello(const char* buf, int start, int end) {
	puts("hello");
}

int number_fn(struct Dent* dent, const char* buf, int start, int end) {
	for (int i = start; i < end; ++i) {
		char c = buf[i];
		if (!isdigit(c)) {
			putchar('[');
			for (int i = start; i < end; ++i) {
				putchar(buf[i]);
			}
			puts("] is not a number!\n");
			return 0;
		}
	}
	putchar('[');
	for (int i = start; i < end; ++i) {
		putchar(buf[i]);
	}
	puts("] is a number!\n");
	return 1;
}

struct Dent dict[1000] = {
		{ 0, number_fn, 0, 0 }
};
struct Dent* head = dict;

void dadd_p(const char* name, typefn type, void* p) {
	struct Dent* dent = head+1;
	dent->name = name;
	dent->type = type;
	dent->value.p = p;
	dent->prev = head;
	++head;
}

int match(const char* defname, const char* buf, int start, int end) {
	if (0 == defname || 0 == *defname) return 1; // NULL or empty is a wildcard which always matches
	int len = end - start;

	for (int i = 0; i < len; ++i) {
		if (defname[i] == 0) return 0;
		if (defname[i] != buf[start+i]) return 0;
	}
	return defname[len] == 0;
}

void eval(const char* word, int start, int end) {
	struct Dent* walk = head;
	do {
		if (match(walk->name, word, start, end)) {
			if (walk->type(walk, word, start, end)) break;
		}
		walk = walk->prev;
	} while (walk != 0);

	if (0 == walk) {
		puts("NOT FOUND");
	}
}

#define OUTSIDE 0
#define INSIDE 1

void input(const char* buffer) {
	int start = 0;
	int end = 0;
	int state = OUTSIDE;
	int i = 0;

	while (buffer[i]) {
		switch(state) {
		case OUTSIDE:
			if (' ' != buffer[i]) {
				end = start = i;
				state = INSIDE;
			}
			break;
		case INSIDE:
			if (' ' == buffer[i]) {
				eval(buffer, start, i);
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
		eval(buffer,start,i);
	}
}

int main(void) {
	dadd_p("?", fn_fn, &hello);
	dadd_p("p", prints_fn, "lala");

	input("23 skidoo");

	dadd_p("skidoo", prints_fn, "honey");

	input("23 skidoo");

	return 0;
}
