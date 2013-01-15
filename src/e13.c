#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

struct Dent;

typedef void (*fn)(const char* word);
typedef int (*typefn)(struct Dent* dent, const char* word); // return 1 if handled, 0 otherwise

struct Dent {
	const char* name;
	typefn type;
	union { void* p; int v; } value;
	struct Dent* prev;
};

void nop_fn(struct Dent* dent, const char* word) {}

int fn_fn(struct Dent* dent, const char* word) {
	((fn)dent->value.p)(word);
	return 1;
}

int prints_fn(struct Dent* dent, const char* word) {
	printf("%s\n", dent->value.p);
	return 1;
}

void hello(const char* word) {
	puts("hello");
}

int number_fn(struct Dent* dent, const char* word) {
	const char* p = word;
	while (*p != 0) {
		if (!isdigit(*p++)) {
			printf("[%s] is not a number!\n", word);
			return 0;
		}
	}
	printf("[%s] is a number!\n", word);
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

void eval(const char* word) {
	struct Dent* walk = head;
	do {
		if (0 == walk->name) {
			if (walk->type(walk, word)) break;
		} else if (strcmp(word, walk->name) == 0) {
			walk->type(walk, word);
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
	eval("123");
	return 0;
}
