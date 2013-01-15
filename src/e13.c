#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct Dent;

typedef void (*fn)(void);
typedef void (*typefn)(struct Dent* dent);

struct Dent {
	const char* name;
	typefn type;
	union { void* p; int v; } value;
	struct Dent* prev;
};

void nop_fn(struct Dent* dent) {}

void fn_fn(struct Dent* dent) {
	((fn)dent->value.p)();
}

void prints_fn(struct Dent* dent) {
	printf("%s\n", dent->value.p);
}

void hello(void) {
	puts("hello");
}

struct Dent dict[1000] = {
		{ "", nop_fn, 0, 0 }
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


void eval(const char* word) {
	struct Dent* walk = head;
	do {
		if (strcmp(word, walk->name) == 0) {
			walk->type(walk);
			break;
		}
		walk = walk->prev;
	} while (walk != 0);

	if (0 == walk) {
		puts("NOT FOUND");
	}
}

int main(void) {
	dadd_p("?", fn_fn, &hello);
	dadd_p("p", prints_fn, "lala");

	eval("?");
	eval("p");
	eval("q");
	return 0;
}
