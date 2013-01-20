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

static void bytes_read_write() {
  fail_unless(byte_read(POOL_NEXT) == 0, "pool next should be 0 at start");
  byte_write(POOL_NEXT, 123);
  fail_unless(byte_read(POOL_NEXT) == 123, "pool next should contain the updated value");
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

  word reset_bytes[12] = {
      0,0,0,0,  0,0,0,1,  ' ',0,0,0
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
  POOL_NEXT = POOL_START+12;
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
  test(bytes_read_write);

  if (fails) {
    printf("%d tests failed\n", fails);
  } else {
    puts("all tests passed");
  }
  return fails;
}
