#ifndef E13_H
#define E13_H

#include <stdint.h>
#include <limits.h>

#if UINTPTR_MAX == 0xffff
#define WORDSIZE 2
#endif
#if UINTPTR_MAX == 0xffffffff
#define WORDSIZE 4
#endif
#if UINTPTR_MAX == 0xffffffffffffffff
#define WORDSIZE 8
#endif

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

struct sys_const {
  address inbuf_start;
  address inbuf_end;
  address dstack_start;
  address dstack_end;
  address rstack_start;
  address rstack_end;
  address dict_start;
  address dict_end;
  address scratch_start;
  address scratch_end;
  address pool_start;
  address pool_end;
};

#define sys_const(name) ((struct sys_const*)memory_start)->name
#define INBUF_START sys_const(inbuf_start)
#define INBUF_END sys_const(inbuf_end)
#define DSTACK_START sys_const(dstack_start)
#define DSTACK_END sys_const(dstack_end)
#define RSTACK_START sys_const(rstack_start)
#define RSTACK_END sys_const(rstack_end)
#define DICT_START sys_const(dict_start)
#define DICT_END sys_const(dict_end)
#define SCRATCH_START sys_const(scratch_start)
#define SCRATCH_END sys_const(scratch_end)
#define POOL_START sys_const(pool_start)
#define POOL_END sys_const(pool_end)

struct sys_var {
  address ds_top;
  address rs_top;
  address dict_head;
  address dict_next;
  address pool_head;
  address pool_next;
  address inbuf_in;
  address inbuf_out;
};

#define sys_var(name) ((struct sys_var*)(memory_start + sizeof(struct sys_const)))->name
#define DS_TOP sys_var(ds_top)
#define RS_TOP sys_var(rs_top)
#define DICT_HEAD sys_var(dict_head)
#define DICT_NEXT sys_var(dict_next)
#define POOL_HEAD sys_var(pool_head)
#define POOL_NEXT sys_var(pool_next)
#define INBUF_IN sys_var(inbuf_in)
#define INBUF_OUT sys_var(inbuf_out)


// memory model sizes, adjusting these should be safe, just keep them all on WORDSIZE-byte boundaries
#define MEMORY_SIZE 65536
#define CONST_BYTES sizeof(struct sys_const)
#define VAR_BYTES sizeof(struct sys_var)
#define INBUF_BYTES 1024
#define DSTACK_WORDS 256
#define RSTACK_WORDS 256
#define DICT_WORDS 256
#define SCRATCH_BYTES 1024
#define POOL_BYTES (MEMORY_SIZE - (INBUF_BYTES) - (DICT_WORDS*WORDSIZE) - (RSTACK_WORDS*WORDSIZE) - (DSTACK_WORDS*WORDSIZE) - CONST_BYTES - VAR_BYTES)

extern byte* memory_start;

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

// memory access functions
void push(word v);
word pop();
void rpush(address v);
address rpop();

byte* real_address(address a);

typedef void (*typefn)(word param);
typedef void (*primfn)(void);

// data manipulation functions

// add a string to the pool and return its address
address padd(address start, word length);

// lookup a string in the pool and return its address or NOT_FOUND if not found
address plup(address start, word length);

// lookup a symbol in the dictionary and return its address or NOT_FOUND if not found
address dlup(address symbol);

// execute a dictionary entry
void execute(typefn fn, word value);

// evaluate a sequence of words, either from input, or from the pool
void evaluate(address p, word length);
void evaluate_pent(address p);

/* candidates for re-implementation as compiled words */

// attempt to parse and push a number from a string
int number(address start, int length);

// lookup a string in the pool and return its address or add it if not found
address pens(address start, word length);

#if TEST
#define byte_read(p) _byte_read(p, __FUNCTION__, __LINE__)
#define byte_write(p,v) _byte_write(p, v, __FUNCTION__, __LINE__)
#define word_read(p) _word_read(p, __FUNCTION__, __LINE__)
#define word_write(p,v) _word_write(p, v, __FUNCTION__, __LINE__)

byte _byte_read(address p, const char* f, int r);
void _byte_write(address p, byte v, const char* f, int r);
word _word_read(address p, const char* f, int r);
void _word_write(address p, word v, const char* f, int r);
#else
byte byte_read(address p);
void byte_write(address p, byte v);
word word_read(address p);
void word_write(address p, word v);
#endif

#endif
