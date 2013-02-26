#include <stdio.h>

#include "e13.h"
#include "debug.h"

static int fails = 0;

int _fail_if(const char* fn, int line, int expr, const char* message) {
  if (expr) {
    printf("%s:%d %s\n", fn, line, message);
    ++fails;
    return 1;
  }

  return 0;
}

#define fail_if(expr, message) if (_fail_if(__FUNCTION__, __LINE__, expr, message)) return
#define complain(message) _fail_if(__FUNCTION__, __LINE__, 1, message)
#define fail_unless(expr, message) fail_if(!(expr), message)
#define fail(message) fail_if(1, message)

#define START //printf("%s:start\n", __FUNCTION__);
#define END //printf("%s:end\n", __FUNCTION__);

// the memory model, simulating basic RAM so that I can get as close to Chuck's original design as possible.
byte bytes[MEMORY_SIZE];

byte* memory_start = bytes;

// memory access functions, with extra protection for tests
uint8_t _byte_read(address p, const char* f, int r) {
  if (p < (word)bytes || p >= (word)(bytes + MEMORY_SIZE)) {
    printf("attempt to read byte outside memory map[%lx-%lx] (p=%lx) fn=[%s] line=%d\n", bytes, bytes + MEMORY_SIZE, (word)p, f, r);
    return 0;
  }
  return *(byte*)p;
}
void _byte_write(address p, byte v, const char* f, int r) {
  if (p < (word)bytes || p >= (word)(bytes + MEMORY_SIZE)) {
    complain("attempt to write byte outside memory map");
  }
  *(byte*)p = v;
}
word _word_read(address p, const char* f, int r) {
  if (p < (word)bytes || p >= (word)(bytes + MEMORY_SIZE)) {
    complain("attempt to read word outside memory map");
    return 0;
  }
  word ret = 0;
  ret += ((word)byte_read(p+0));
  ret += ((word)byte_read(p+1))<<8;
#if WORDSIZE > 2
  ret += ((word)byte_read(p+2))<<16;
  ret += ((word)byte_read(p+3))<<24;
#if WORDSIZE > 4
  ret += ((word)byte_read(p+4))<<32;
  ret += ((word)byte_read(p+5))<<40;
  ret += ((word)byte_read(p+6))<<48;
  ret += ((word)byte_read(p+7))<<56;
#endif
#endif
//printf("word_read(p=%d)=>%d\n", p, ret);
  return ret;
}
void _word_write(address p, word v, const char* f, int r) {
  if (p < (word)bytes || p >= (word)(bytes + MEMORY_SIZE)) {
    complain("attempt to write word outside memory map");
  }
//printf("word_write(p=%d,v=%d)\n", p, v);
  byte_write(p+0, (v & 0x00FF));
  byte_write(p+1, (v & 0xFF00) >> 8);
#if WORDSIZE > 2
  byte_write(p+2, (v & 0x00FF0000) >> 16);
  byte_write(p+3, (v & 0xFF000000) >> 24);
#if WORDSIZE > 4
  byte_write(p+4, (v & 0x000000FF00000000) >> 32);
  byte_write(p+5, (v & 0x0000FF0000000000) >> 40);
  byte_write(p+6, (v & 0x00FF000000000000) >> 48);
  byte_write(p+7, (v & 0xFF00000000000000) >> 56);
#endif
#endif
}

void dnext(void) {
  address old_next = DICT_NEXT;
  DICT_HEAD = old_next;

  DICT_NEXT += DENT_SIZE;
  word_write(DICT_NEXT+DENT_NAME, 0);
  word_write(DICT_NEXT+DENT_TYPE, 0);
  word_write(DICT_NEXT+DENT_PARAM, 0);
  word_write(DICT_NEXT+DENT_PREV, old_next);
}

void type(const char* s) {
  INBUF_IN = INBUF_START;
  for (int i = 0; s[i] != 0; ++i) {
    byte_write(INBUF_IN++, s[i]);
  }
  byte_write(INBUF_IN, 0);
}

void dadd(const char* name, address typefn, word param) {
  type(name);
//printf("dadd: INBUF_START=%x, INBUF_IN=%x, length=%d\n", INBUF_START, INBUF_IN, INBUF_IN-INBUF_START);
  word_write(DICT_NEXT+DENT_NAME, pens(INBUF_START, INBUF_IN-INBUF_START));
  word_write(DICT_NEXT+DENT_TYPE, typefn);
  word_write(DICT_NEXT+DENT_PARAM, param);
  dnext();
}

// set up default entries and initialise variables
void init() {
  // set "constants"
  INBUF_START = (address)(memory_start + sizeof(struct sys_const) + sizeof(struct sys_var));
  INBUF_END = INBUF_START + INBUF_BYTES;

  DSTACK_START = INBUF_END;
  DSTACK_END = DSTACK_START + DSTACK_WORDS * WORDSIZE;

  RSTACK_START = DSTACK_END;
  RSTACK_END = RSTACK_START + RSTACK_WORDS * WORDSIZE;

  DICT_START = RSTACK_END;
  DICT_END = DICT_START + DICT_WORDS * WORDSIZE;

  SCRATCH_START = DICT_END;
  SCRATCH_END = SCRATCH_START + SCRATCH_BYTES;

  POOL_START = SCRATCH_END;
  POOL_END = POOL_START + POOL_BYTES;

  // set "variables"
  INBUF_IN = INBUF_START;
  INBUF_OUT = INBUF_START;

  DS_TOP = DSTACK_START;

  POOL_NEXT = POOL_START;
  POOL_HEAD = POOL_START;

  DICT_NEXT = DICT_START;
  dadd("", (word)&number, 0);

  RS_TOP = RSTACK_START;
}

void evaluate_INBUF(void) {
  evaluate(INBUF_START, INBUF_IN);
}

void enter(const char* s) {
  type(s);
  evaluate_INBUF();
}

static void data_stack() {
  START
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at start");
  push(1234);
  fail_unless(DS_TOP > DSTACK_START, "after push, stack should be bigger");
  fail_unless(1234 == pop(), "pop should fetch the pushed value");
  fail_unless(DS_TOP == DSTACK_START, "after pop, stack should be empty again");
  END
}

static void return_stack() {
  START
  fail_unless(RS_TOP == RSTACK_START, "stack should be empty at start");
  rpush(1234);
  fail_unless(RS_TOP > RSTACK_START, "after push, stack should be bigger");
  fail_unless(1234 == rpop(), "pop should fetch the pushed value");
  fail_unless(RS_TOP == RSTACK_START, "after pop, stack should be empty again");
  END
}

static void dict_read_write() {
  START
  fail_unless(DICT_NEXT > DICT_HEAD, "dict should be initialised before start");
  fail_unless(word_read(DICT_NEXT+DENT_NAME) == 0, "dict[next] name should be 0 at start");
  fail_unless(word_read(DICT_NEXT+DENT_TYPE) == 0, "dict[next] type fn should be 0 at start");
  fail_unless(word_read(DICT_NEXT+DENT_PARAM) == 0, "dict[next] param should be 0 at start");
  fail_unless(word_read(DICT_NEXT+DENT_PREV) == DICT_HEAD, "dict[next] prev should point back to head at start");
  word_write(DICT_NEXT+DENT_NAME, 123);
  fail_unless(word_read(DICT_NEXT+DENT_NAME) == 123, "dict[next] name should change when written");
  END
}

static void dict_move_to_next() {
  START
  dict_read_write();
  word_write(DICT_NEXT+DENT_TYPE, 456);
  fail_unless(word_read(DICT_NEXT+DENT_TYPE) == 456, "dict[next] type should change when written");
  word_write(DICT_NEXT+DENT_PARAM, 789);
  fail_unless(word_read(DICT_NEXT+DENT_PARAM) == 789, "dict[next] type should change when written");

  address old_head = DICT_HEAD;
  address old_next = DICT_NEXT;

  dnext();

  fail_unless(DICT_HEAD == old_next, "dict head should now be what was DICT_NEXT");
  fail_unless(word_read(DICT_HEAD+DENT_NAME) == 123, "dict[head] name should be as defined");
  fail_unless(word_read(DICT_HEAD+DENT_TYPE) == 456, "dict[head] type should be as defined");
  fail_unless(word_read(DICT_HEAD+DENT_PARAM) == 789, "dict[head] param should be as defined");
  fail_unless(word_read(DICT_HEAD+DENT_PREV) == old_head, "dict[head] prev should point back to old head");
  fail_unless(DICT_NEXT > DICT_HEAD, "dict next should still be more than head");

  fail_unless(word_read(DICT_NEXT+DENT_NAME) == 0, "new dict[next] name should be 0");
  fail_unless(word_read(DICT_NEXT+DENT_TYPE) == 0, "new dict[next] type fn should be 0");
  fail_unless(word_read(DICT_NEXT+DENT_PARAM) == 0, "new dict[next] param should be 0");
  fail_unless(word_read(DICT_NEXT+DENT_PREV) == DICT_HEAD, "new dict[next] prev should point back to new head");
  END
}

static void dict_lookup() {
  START
  fail_unless(dlup(123) == NOT_FOUND, "dict lookup should not find undefined item");
  word_write(DICT_NEXT+DENT_NAME, 123);
  dnext();
  fail_unless(dlup(123) == DICT_HEAD, "dict lookup should find item at head");

  address first_match = DICT_HEAD;
  word_write(DICT_NEXT+DENT_NAME, 456);
  dnext();
  fail_unless(dlup(123) == first_match, "dict lookup should find item behind head");

  word_write(DICT_NEXT+DENT_NAME, 123);
  dnext();
  fail_unless(dlup(123) != first_match, "dict lookup should find override");
  END
}

static void pool_read_write() {
  START
  fail_unless(byte_read(POOL_NEXT) == 0, "pool next should be 0 at start");
  byte_write(POOL_NEXT, 123);
  fail_unless(byte_read(POOL_NEXT) == 123, "pool next should contain the updated value");

  word_write(POOL_NEXT, 0xaabbccdd);
  fail_unless(byte_read(POOL_NEXT+0) == 0xdd, "pool should contain stored word (byte 0)");
  fail_unless(byte_read(POOL_NEXT+1) == 0xcc, "pool should contain stored word (byte 1)");
  fail_unless(byte_read(POOL_NEXT+2) == 0xbb, "pool should contain stored word (byte 2)");
  fail_unless(byte_read(POOL_NEXT+3) == 0xaa, "pool should contain stored word (byte 3)");
  fail_unless(word_read(POOL_NEXT) == 0xaabbccdd, "word_read should read the stored value");
  END
}

static void pool_add() {
  START
  fail_unless(POOL_NEXT > POOL_HEAD, "pool next should be beyond pool head");

  address old_head = POOL_HEAD;
  address old_next = POOL_NEXT;
  type("a");
  address added = padd(INBUF_START, INBUF_IN-INBUF_START);
  fail_unless(POOL_HEAD == old_next, "pool head should me moved to old next");
  fail_unless(added == POOL_HEAD, "badd should return new address");
  fail_unless(1 == word_read(POOL_HEAD + PENT_LEN), "supplied length should be stored with text");
  fail_unless(POOL_HEAD + PENT_DATA + WORDSIZE == word_read(POOL_HEAD + PENT_NEXT), "next address should be stored with text");
  fail_unless(POOL_NEXT == POOL_HEAD + PENT_DATA + WORDSIZE, "pool next should be rounded up to the start of the next word block");
  END
}

static void pool_lookup_not_found() {
  START
  type("");
  fail_unless(plup(INBUF_START, INBUF_IN-INBUF_START) == POOL_START, "pool lookup of empty string should return 0");
  type("x");
  fail_unless(plup(INBUF_START, INBUF_IN-INBUF_START) == NOT_FOUND, "pool lookup of undefined string should return NOT_FOUND");
  END
}

static void pool_lookup_found_when_added() {
  START
  type("a");
  fail_unless(plup(INBUF_START, INBUF_IN-INBUF_START) == NOT_FOUND, "pool lookup of undefined string should return NOT_FOUND");

  address added = padd(INBUF_START, INBUF_IN-INBUF_START);
  address found = plup(INBUF_START, INBUF_IN-INBUF_START);
  fail_unless(found == added, "pool lookup of defined string should return its address");
  END
}

static void pool_lookup_found_by_length() {
  START
  type("x");
  fail_unless(plup(INBUF_START, INBUF_IN-INBUF_START) == NOT_FOUND, "pool lookup of undefined string should return NOT_FOUND");

  type("ab");
  address added1 = padd(INBUF_START, INBUF_IN-INBUF_START);
  type("a");
  fail_unless(plup(INBUF_START, INBUF_IN-INBUF_START) == NOT_FOUND, "pool lookup of partial string should return NOT_FOUND");

  address added2 = padd(INBUF_START, INBUF_IN-INBUF_START);
  fail_unless(plup(INBUF_START, INBUF_IN-INBUF_START) == added2, "pool lookup of defined string should return its address");
  END
}

static void pool_lookup_found_by_content() {
  START
  type("x");
  fail_unless(plup(INBUF_START, INBUF_IN-INBUF_START) == NOT_FOUND, "pool lookup of undefined string should return NOT_FOUND");

  address added1 = padd(INBUF_START, INBUF_IN-INBUF_START);

  type("y");
  fail_unless(plup(INBUF_START, INBUF_IN-INBUF_START) == NOT_FOUND, "pool lookup of partial string should return NOT_FOUND");

  address added2 = padd(INBUF_START, INBUF_IN-INBUF_START);
  fail_unless(plup(INBUF_START, INBUF_IN-INBUF_START) == added2, "pool lookup of defined string should return its address");
  END
}

static void pool_ensure() {
  START
  type("x");
  address old_next = POOL_NEXT;
  address created = pens(INBUF_START, INBUF_IN-INBUF_START);
  fail_unless(created == old_next, "pool ensure of unknown string should create it");
  type("x");
  address found =  pens(INBUF_START, INBUF_IN-INBUF_START);
  fail_unless(found == created, "pool ensure of created string should find it");
  END
}

static void not_a_number() {
  START
  type("");
  fail_unless(0 == number(INBUF_START, INBUF_IN-INBUF_START), "empty string should not be a number");
  fail_unless(DS_TOP == DSTACK_START, "empty string should not push");

  type("x");
  fail_unless(0 == number(INBUF_START, INBUF_IN-INBUF_START), "'x' should not be a number");
  fail_unless(DS_TOP == DSTACK_START, "non-number string should not push");
  END
}

static void positive_number() {
  START
  type("1");
  fail_unless(1 == number(INBUF_START, INBUF_IN-INBUF_START), "'1' should be a number");
  fail_unless(DS_TOP != DSTACK_START, "number string should push");
  fail_unless(1 == pop(), "number should be pushed");

  type("12");
  fail_unless(1 == number(INBUF_START, INBUF_IN-INBUF_START), "'12' should be a number");
  fail_unless(DS_TOP != DSTACK_START, "number string should push");
  fail_unless(12 == pop(), "number should be pushed");

  type("123");
  fail_unless(1 == number(INBUF_START, INBUF_IN-INBUF_START), "'123' should be a number");
  fail_unless(DS_TOP != DSTACK_START, "number string should push");
  fail_unless(123 == pop(), "number should be pushed");

  type("123p");
  fail_unless(0 == number(INBUF_START, INBUF_IN-INBUF_START), "'123p' should not be a number");
  fail_unless(DS_TOP == DSTACK_START, "non-number string should not push");
  END
}

static void negative_number() {
  START
  type("-");
  fail_unless(0 == number(INBUF_START, INBUF_IN-INBUF_START), "'-' should not be a number");
  fail_unless(DS_TOP == DSTACK_START, "non-number string should not push");

  type("-2");
  fail_unless(1 == number(INBUF_START, INBUF_IN-INBUF_START), "'-2' should be a number");
  fail_unless(DS_TOP != DSTACK_START, "number string should push");
  fail_unless(-2 == pop(), "number should be pushed");

  type("-23");
  fail_unless(1 == number(INBUF_START, INBUF_IN-INBUF_START), "'-23' should be a number");
  fail_unless(DS_TOP != DSTACK_START, "number string should push");
  fail_unless(-23 == pop(), "number should be pushed");
  END
}

static void eval_empty() {
  START
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at start");
  byte_write(INBUF_START, 0);
  evaluate(INBUF_START, INBUF_START);
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at end, too");
  END
}

static void eval_number() {
  START
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at start");
  enter("23");
  fail_unless(DS_TOP >DSTACK_START, "stack should not be empty at end");
  fail_unless(23 == pop(), "parameter value should have been pushed");
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at end, too");
  END
}

static void eval_two_numbers() {
  START
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at start");
  enter("23 97");
  fail_unless(DS_TOP > DSTACK_START, "stack should not be empty at end");
  fail_unless(97 == pop(), "parameter value 97 should have been pushed");
  fail_unless(23 == pop(), "parameter value 23 should have been pushed");
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at end, too");
  END
}

static void eval_string() {
  START
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at start");
  enter("[hello]");

  fail_unless(DS_TOP >DSTACK_START, "stack should not be empty at end");
  word pushed = pop();
  fail_unless(0 != pushed, "pushed should be non-zero");
  fail_unless(NOT_FOUND != pushed, "pushed should not be NOT_FOUND");

  type("hello");
  fail_unless(plup(INBUF_START, INBUF_IN-INBUF_START) == pushed, "string address should have been pushed");
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at end, too");
  END
}

void dup(word param) {
  word x = pop();
  push(x);
  push(x);
}

static void eval_word() {
  START
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at start");

  enter("96 hello");
  fail_unless(DS_TOP > DSTACK_START, "stack should not be empty at end");
  fail_unless(96 == pop(), "parameter value 96 should have been pushed once");
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at end, too");

  type("hello");
  address name = padd(INBUF_START, INBUF_IN-INBUF_START);
  word_write(DICT_NEXT+DENT_NAME, name);
  word_write(DICT_NEXT+DENT_TYPE, (word)&dup);
  word_write(DICT_NEXT+DENT_PARAM, 0);
  dnext();

  enter("96 hello");
  fail_unless(DS_TOP > DSTACK_START, "stack should not be empty at end");
  fail_unless(96 == pop(), "parameter value 96 should have been pushed once");
  fail_unless(96 == pop(), "parameter value 96 should have been pushed twice");
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at end, too");
  END
}

void define(const char* names, const char* bodys) {
  type(names);
  word name = pens(INBUF_START, INBUF_IN-INBUF_START);
  type(bodys);
  word body = pens(INBUF_START, INBUF_IN-INBUF_START);

  word_write(DICT_NEXT+DENT_NAME, name);
  word_write(DICT_NEXT+DENT_TYPE, (word)&evaluate_pent);
  word_write(DICT_NEXT+DENT_PARAM, body);
  dnext();
}

static void eval_subroutine() {
  START
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at start");

  enter("96 hello");
  fail_unless(DS_TOP > DSTACK_START, "stack should not be empty at end");
  fail_unless(96 == pop(), "parameter value 96 should have been pushed once");
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at end, too");

  define("hello", "23 45");

  enter("96 hello");

  fail_unless(DS_TOP > DSTACK_START, "stack should not be empty at end");
  fail_unless(45 == pop(), "parameter value 45 should have been pushed");
  fail_unless(23 == pop(), "parameter value 23 should have been pushed");
  fail_unless(96 == pop(), "parameter value 96 should have been pushed");
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at end, too");
  END
}

void primitive(address p) {
  ((primfn)p)();
}

void prim_b_plus() {
  push(pop() + (word)1);
}

void prim_w_plus() {
  push(pop() + WORDSIZE);
}

void prim_b_read() {
  push(byte_read(pop()));
}

void prim_b_write() {
  byte_write(pop(), pop());
}

void prim_w_read() {
  push(word_read(pop()));
}

void prim_w_write() {
  word_write(pop(), pop());
}

static void eval_prims() {
  START
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at start");

  dadd("B+", (address)&primitive, (address)&prim_b_plus);
  dadd("W+", (address)&primitive, (address)&prim_w_plus);
  dadd("B@", (address)&primitive, (address)&prim_b_read);
  dadd("B!", (address)&primitive, (address)&prim_b_write);
  dadd("W@", (address)&primitive, (address)&prim_w_read);
  dadd("W!", (address)&primitive, (address)&prim_w_write);
//dump_dict();

  enter("96 B+");
  fail_unless(DS_TOP > DSTACK_START, "stack should not be empty at end");
  fail_unless(97 == pop(), "parameter value 96 should have been incremented by one");
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at end, too");

  enter("96 W+");
  fail_unless(DS_TOP > DSTACK_START, "stack should not be empty at end");
  fail_unless(96 + WORDSIZE == pop(), "parameter value 96 should have been incremented by WORDSIZE");
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at end, too");
}

int reset() {
  //  printf("*"); fflush(stdout);
  init();
  return 1;
}

#define test(fn) if (reset()) fn()

int main() {
  printf("running tests with WORDSIZE [%d]\n", WORDSIZE);

  test(data_stack);
  test(return_stack);
  test(dict_read_write);
  test(dict_move_to_next);
  test(dict_lookup);
  test(pool_read_write);
  test(pool_add);
  test(pool_lookup_not_found);
  test(pool_lookup_found_when_added);
  test(pool_lookup_found_by_length);
  test(pool_lookup_found_by_content);
  test(pool_ensure);
  test(not_a_number);
  test(positive_number);
  test(negative_number);
  test(eval_empty);
  test(eval_number);
  test(eval_two_numbers);
  test(eval_string);
  test(eval_word);
  test(eval_subroutine);
  test(eval_prims);

  if (fails) {
    printf("%d tests failed\n", fails);
  } else {
    puts("all tests passed");
  }
  return fails;
}
