#ifndef E13_H
#define E13_H

#include <stdint.h>

// memory model sizes, adjusting these should be safe, just keep them all on 4-byte boundaries
#define DSTACK_WORDS 256
#define RSTACK_WORDS 256
#define DICT_WORDS 256
#define INRING_BYTES 1024
#define POOL_BYTES (65536 - (INRING_BYTES) - (DICT_WORDS*4) - (RSTACK_WORDS*4) - (DSTACK_WORDS*4))

// address constants, referring to memory blocks etc.
// for development each of the memory blocks is separate and relative.
// For deployment they should be absolute
#define DSTACK_START 0
#define DSTACK_END DSTACK_WORDS
#define RSTACK_START 0
#define RSTACK_END RSTACK_WORDS
#define DICT_START 0
#define DICT_END DICT_WORDS
#define POOL_START 0
#define POOL_END (POOL_START + POOL_BYTES)
#define INRING_START POOL_END
#define INRING_END (INRING_START + RING_BYTES)

// field offsets
#define DENT_NAME 0
#define DENT_TYPE 1
#define DENT_PARAM 2
#define DENT_PREV 3

#define PENT_LEN 0
#define PENT_DATA 4

// synbolic constants
#define OUTSIDE 0
#define INSIDE 1

typedef uint32_t address;
typedef uint32_t word;
typedef uint8_t byte;

// the memory model, simulating basic RAM so that I can get as close to Chuck's original design as possible.
extern word dstack[];
extern word rstack[];
extern word dict[];
extern byte bytes[];

// system variables, really these belong in memory, perhaps before DSTACK
extern address DS_TOP;
extern address RS_TOP;
extern address DICT_HEAD;
extern address DICT_NEXT;
extern address POOL_HEAD;
extern address POOL_NEXT;
extern address RING_IN;
extern address RING_OUT;
extern word INPUT_COUNT;

// memory access functions
void push(word v);
word pop();
void rpush(address v);
address rpop();
word dict_read(address p);
void dict_write(address p, word v);

byte byte_read(address p);
void byte_write(address p, byte v);

word word_read(address p);
void word_write(address p, word v);

typedef int (*typefn)(address dent); // return 1 if handled, 0 otherwise

// data manipulation functions

// lookup a string in the pool and return its address or FFFFFFFF if not found
address blookup(address start, int length);

// lookup a string in the pool and return its address or add it if not found
address badd(address start, int length);

// lookup a symbol in the dictionary and return its address or FFFFFFFF if not found
address dlookup(address symbol);

// advance the head of the dictionary after a new top entry has been populated
void dadd(void);

// execute a dictionary entry
void execute(address);

// evaluate a sequence of words, either from the inring, or from the pool
void eval(address p, int length);

// initialise the system variables and initial dict entries
void init(void);

// run accepting and processing input
void run(void);

// attempt to parse and push a number from a string
int number(address start, int len);

#endif
