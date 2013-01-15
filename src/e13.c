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
	union { void* p; uint32_t v; } value;
	struct Dent* prev;
};


void fn_fn(struct Dent* dent) {
	((fn)dent->value.p)();
}

void prints_fn(struct Dent* dent) {
	printf("%s\n", dent->value.p);
}

void hello(void) {
	puts("hello");
}

void goodbye(void) {
	puts("goodbye");
}

struct Dent dict[1000] = {
		{ },
		{ "?", fn_fn, {.p = &hello}, 0 },
		{ "p", prints_fn, {.p = "lala"}, 1}
};
int head = 2;

void eval(const char* word) {
	int walk = head;
	do {
		struct Dent* def = &dict[walk];
		if (strcmp(word, def->name) == 0) {
			def->type(def);
			break;
		}
		walk = def->prev;
	} while (walk != 0);

	if (0 == walk) {
		puts("NOT FOUND");
	}
}

int main(void) {
	eval("?");
	eval("p");
	eval("q");
	return 0;
}
