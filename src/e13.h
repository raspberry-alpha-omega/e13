#ifndef E13_H
#define E13_H

#include <stdint.h>
#include <limits.h>

#if UINTPTR_MAX == 0xffffffff
#define WORDSIZE 4
#endif
#if UINTPTR_MAX == 0xffffffffffffffff
#define WORDSIZE 8
#endif

// memory model sizes, adjusting these should be safe, just keep them all on WORDSIZE-byte boundaries
#define MEMORY_SIZE 65536

#define INBUF_BYTES 1024
#define DSTACK_WORDS 256
#define RSTACK_WORDS 256
#define DICT_WORDS 256
#define SCRATCH_BYTES 1024
#define POOL_BYTES (MEMORY_SIZE - (INBUF_BYTES) - (DICT_WORDS*WORDSIZE) - (RSTACK_WORDS*WORDSIZE) - (DSTACK_WORDS*WORDSIZE))

// address constants, referring to memory blocks etc.
// for development each of the memory blocks is separate and relative.
// For deployment they should be absolute
#define MEMORY_START 0

#define INBUF_START MEMORY_START
#define INBUF_END (INBUF_START + INBUF_BYTES)

#define DSTACK_START INBUF_END
#define DSTACK_END (DSTACK_START + (DSTACK_WORDS * WORDSIZE))

#define RSTACK_START DSTACK_END
#define RSTACK_END (RSTACK_START + (RSTACK_WORDS * WORDSIZE))

#define DICT_START RSTACK_END
#define DICT_END (DICT_START + (DICT_WORDS * WORDSIZE))

#define SCRATCH_START DICT_END
#define SCRATCH_END (SCRATCH_START + SCRATCH_BYTES)

#define POOL_START SCRATCH_END
#define POOL_END (POOL_START + POOL_BYTES)

// field offsets
#define DENT_NAME 0
#define DENT_TYPE (1 * WORDSIZE)
#define DENT_PARAM (2 * WORDSIZE)
#define DENT_PREV (3 * WORDSIZE)
#define DENT_SIZE (4 * WORDSIZE)

#define PENT_LEN 0
#define PENT_NEXT (1 * WORDSIZE)
#define PENT_DATA (2 * WORDSIZE)

// synbolic constants
#define OUTSIDE 0
#define INSIDE 1
#define INSTRING 2

#define NOT_FOUND UINTPTR_MAX
#define STRING_START '['
#define STRING_END ']'

typedef uint8_t byte;

#if (WORDSIZE == 2)
typedef uint16_t address;
typedef uint16_t word;
#endif
#if (WORDSIZE == 4)
typedef uint32_t address;
typedef uint32_t word;
#endif
#if (WORDSIZE == 8)
typedef uint64_t address;
typedef uint64_t word;
#endif

// the memory model, simulating basic RAM so that I can get as close to Chuck's original design as possible.
extern byte bytes[];

// system variables, really these belong in memory, perhaps before DSTACK
extern address DS_TOP;
extern address RS_TOP;
extern address DICT_HEAD;
extern address DICT_NEXT;
extern address POOL_HEAD;
extern address POOL_NEXT;
extern address INBUF_IN;
extern address INBUF_OUT;

// memory access functions
void push(word v);
word pop();
void rpush(address v);
address rpop();

byte byte_read(address p);
void byte_write(address p, byte v);

word word_read(address p);
void word_write(address p, word v);

typedef void (*typefn)(word param);
typedef void (*primfn)(void);

word roundup(address p);

// data manipulation functions

// lookup a string in the pool and return its address or NOT_FOUND if not found
address blookup(address start, word length);

// lookup a symbol in the dictionary and return its address or NOT_FOUND if not found
address dlookup(address symbol);

// execute a dictionary entry
void execute(typefn fn, word value);

// evaluate a sequence of words, either from input, or from the pool
void evaluate(address p, word length);
void evaluate_pent(address p);

// initialise the system variables and initial dict entries
void init(void);

// run accepting and processing input
void run(void);



/* candidates for re-implementation as compiled words */

// attempt to parse and push a number from a string
int number(address start);

// lookup a string in the pool and return its address or add it if not found
address badd(address start);

// advance the head of the dictionary after a new top entry has been populated
void dnext(void);

#endif
