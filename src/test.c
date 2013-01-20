#include <stdio.h>

#include "e13.h"

static int fails = 0;

#define fail_if(expr, message) if (_fail_if(__FUNCTION__, __LINE__, expr, message)) return
#define fail_unless(expr, message) fail_if(!(expr), message)
#define fail(message) fail_if(0, message)

int _fail_if(const char* fn, int line, int expr, const char* message) {
  if (expr) {
    printf("%s:%d %s\n", fn, line, message);
    ++fails;
    return 1;
  }

  return 0;
}

static void data_stack() {
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at start");
  push(1234);
  fail_unless(DS_TOP > DSTACK_START, "after push, stack should be bigger");
  fail_unless(1234 == pop(), "pop should fetch the pushed value");
  fail_unless(DS_TOP == DSTACK_START, "after pop, stack should be empty again");
}

static void return_stack() {
  fail_unless(RS_TOP == RSTACK_START, "stack should be empty at start");
  rpush(1234);
  fail_unless(RS_TOP > DSTACK_START, "after push, stack should be bigger");
  fail_unless(1234 == rpop(), "pop should fetch the pushed value");
  fail_unless(RS_TOP == RSTACK_START, "after pop, stack should be empty again");
}

static void dict_read_write() {
  fail_unless(DICT_NEXT > DICT_HEAD, "dict should be initialised before start");
  fail_unless(dict_read(DICT_NEXT+DENT_NAME) == 0, "dict[next] name should be 0 at start");
  fail_unless(dict_read(DICT_NEXT+DENT_TYPE) == 0, "dict[next] type fn should be 0 at start");
  fail_unless(dict_read(DICT_NEXT+DENT_PARAM) == 0, "dict[next] param should be 0 at start");
  fail_unless(dict_read(DICT_NEXT+DENT_PREV) == DICT_HEAD, "dict[next] prev should point back to head at start");
  dict_write(DICT_NEXT+DENT_NAME, 123);
  fail_unless(dict_read(DICT_NEXT+DENT_NAME) == 123, "dict[next] name should change when written");
}

static void dict_move_to_next() {
  dict_read_write();
  dict_write(DICT_NEXT+DENT_TYPE, 456);
  fail_unless(dict_read(DICT_NEXT+DENT_TYPE) == 456, "dict[next] type should change when written");
  dict_write(DICT_NEXT+DENT_PARAM, 789);
  fail_unless(dict_read(DICT_NEXT+DENT_PARAM) == 789, "dict[next] type should change when written");

  address old_head = DICT_HEAD;
  address old_next = DICT_NEXT;

  dadd();

  fail_unless(DICT_HEAD == old_next, "dict head should now be what was DICT_NEXT");
  fail_unless(dict_read(DICT_HEAD+DENT_NAME) == 123, "dict[head] name should be as defined");
  fail_unless(dict_read(DICT_HEAD+DENT_TYPE) == 456, "dict[head] type should be as defined");
  fail_unless(dict_read(DICT_HEAD+DENT_PARAM) == 789, "dict[head] param should be as defined");
  fail_unless(dict_read(DICT_HEAD+DENT_PREV) == old_head, "dict[head] prev should point back to old head");
  fail_unless(DICT_NEXT > DICT_HEAD, "dict next should still be more than head");

  fail_unless(dict_read(DICT_NEXT+DENT_NAME) == 0, "new dict[next] name should be 0");
  fail_unless(dict_read(DICT_NEXT+DENT_TYPE) == 0, "new dict[next] type fn should be 0");
  fail_unless(dict_read(DICT_NEXT+DENT_PARAM) == 0, "new dict[next] param should be 0");
  fail_unless(dict_read(DICT_NEXT+DENT_PREV) == DICT_HEAD, "new dict[next] prev should point back to new head");
}

static void dict_lookup() {
  fail_unless(dlookup(123) == 0xFFFFFFFF, "dict lookup should not find undefined item");
  dict_write(DICT_NEXT+DENT_NAME, 123);
  dadd();
  fail_unless(dlookup(123) == DICT_HEAD, "dict lookup should find item at head");

  address first_match = DICT_HEAD;
  dict_write(DICT_NEXT+DENT_NAME, 456);
  dadd();
  fail_unless(dlookup(123) == first_match, "dict lookup should find item behind head");

  dict_write(DICT_NEXT+DENT_NAME, 123);
  dadd();
  fail_unless(dlookup(123) != first_match, "dict lookup should find override");
}

static void bytes_read_write() {
  fail_unless(byte_read(POOL_NEXT) == 0, "pool next should be 0 at start");
  byte_write(POOL_NEXT, 123);
  fail_unless(byte_read(POOL_NEXT) == 123, "pool next should contain the updated value");

  word_write(POOL_NEXT, 0xaabbccdd);
  fail_unless(byte_read(POOL_NEXT+0) == 0xdd, "pool bytes should contain stored word (byte 0)");
  fail_unless(byte_read(POOL_NEXT+1) == 0xcc, "pool bytes should contain stored word (byte 1)");
  fail_unless(byte_read(POOL_NEXT+2) == 0xbb, "pool bytes should contain stored word (byte 2)");
  fail_unless(byte_read(POOL_NEXT+3) == 0xaa, "pool bytes should contain stored word (byte 3)");
  fail_unless(word_read(POOL_NEXT) == 0xaabbccdd, "word_read should read the stored value");
}

static void byte_lookup() {
  bytes[INRING_START] = 'x';
  fail_unless(blookup(INRING_START,0) == 0, "bytes lookup of empty string should return 0");
  fail_unless(blookup(INRING_START,1) == 0xFFFFFFFF, "bytes lookup of undefined string should return FFFFFFFF");
}

static void byte_add() {
  byte_lookup();
  fail_unless(POOL_NEXT > POOL_HEAD, "pool next should be beyond pool head");

  address old_head = POOL_HEAD;
  address old_next = POOL_NEXT;
  address added = badd(INRING_START, 1);
  fail_unless(POOL_HEAD == old_next, "pool head should me moved to old next");
  fail_unless(added == POOL_HEAD, "badd should return new address");
  fail_unless(1 == word_read(POOL_HEAD+PENT_LEN), "supplied length should be stored with text");
  fail_unless(POOL_NEXT == POOL_HEAD + 4 + 4, "pool next should be rounded up to the start of the next word block");
  fail_unless(blookup(INRING_START,1) == POOL_HEAD, "bytes lookup of fresh string should return its address");
}

int reset() {
  int i;

  word reset_dict[8] = {
      0, (word)&number_fn, 0, 0,
      0, 0, 0, DICT_START
  };
  for (i = 0; i < 8; ++i) {
    dict[i] = reset_dict[i];
  }
  for (; i < DICT_END; ++i) {
    dict[i] = 0;
  }

  word reset_bytes[8] = {
      0,0,0,0,  0,0,0,0
  };
  for (i = 0; i < 12; ++i) {
    bytes[i] = reset_bytes[i];
  }
  for (; i < POOL_BYTES + INRING_BYTES; ++i) {
    bytes[i] = 0;
  }

  DS_TOP = DSTACK_START;
  RS_TOP = RSTACK_START;
  DICT_HEAD = DICT_START;
  DICT_NEXT = DICT_START+4;
  POOL_HEAD = POOL_START;
  POOL_NEXT = POOL_START+8;
  RING_IN = INRING_START;
  RING_OUT = INRING_START;
  INPUT_COUNT = 0;

  return 1;
}

#define test(fn) if (reset()) fn()
//#define test(fn) fn()

int main() {
  test(data_stack);
  test(return_stack);
  test(dict_read_write);
  test(dict_move_to_next);
  test(dict_lookup);
  test(bytes_read_write);
  test(byte_lookup);
  test(byte_add);

  if (fails) {
    printf("%d tests failed\n", fails);
  } else {
    puts("all tests passed");
  }
  return fails;
}
