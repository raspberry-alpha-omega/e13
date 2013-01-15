#include <stdio.h>
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

int match(const char* defname, const char* buf, int start, int end) {
	if (0 == defname || 0 == *defname) return 1; // NULL or empty is a wildcard which always matches

	for (int i = 0; i < (end - start); ++i) {
		if (*defname == 0) return 0;
		if (*defname != buf[start+i]) return 0;
	}
	return (*defname != 0);
}

void eval(const char* word) {
	struct Dent* walk = head;
	do {
		if (match(walk->name, word, 0, strlen(word))) {
			if (walk->type(walk, word)) break;
		}
		walk = walk->prev;
	} while (walk != 0);

	if (0 == walk) {
		puts("NOT FOUND");
	}
}

void input(const char* buffer) {
	int start = 0;
	int end = 0;
	int i = 0;
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
