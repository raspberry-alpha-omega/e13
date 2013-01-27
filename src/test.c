#include <stdio.h>

#include "e13.h"

static int fails = 0;

#define fail_if(expr, message) if (_fail_if(__FUNCTION__, __LINE__, expr, message)) return
#define fail_unless(expr, message) fail_if(!(expr), message)
#define fail(message) fail_if(0, message)

#define START //printf("%s:start\n", __FUNCTION__);
#define END //printf("%s:end\n", __FUNCTION__);

void dump_stack(void) {
  printf("stack[ ");
  for (int i = DSTACK_START; i < DS_TOP; ++i) {
    printf("%d ", dstack[i]);
  }
  printf("]\n");
}

void type(const char* s) {
  INBUF_IN = INBUF_START;
  for (int i = 0; s[i] != 0; ++i) {
    byte_write(INBUF_IN++, s[i]);
  }
  byte_write(INBUF_IN++, 0);
}

void evaluate_INBUF(void) {
  evaluate(INBUF_START);
}

void enter(const char* s) {
  type(s);
  evaluate_INBUF();
}

int _fail_if(const char* fn, int line, int expr, const char* message) {
  if (expr) {
    printf("%s:%d %s\n", fn, line, message);
    ++fails;
    return 1;
  }

  return 0;
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
  fail_unless(RS_TOP > DSTACK_START, "after push, stack should be bigger");
  fail_unless(1234 == rpop(), "pop should fetch the pushed value");
  fail_unless(RS_TOP == RSTACK_START, "after pop, stack should be empty again");
  END
}

static void dict_read_write() {
  START
  fail_unless(DICT_NEXT > DICT_HEAD, "dict should be initialised before start");
  fail_unless(dict_read(DICT_NEXT+DENT_NAME) == 0, "dict[next] name should be 0 at start");
  fail_unless(dict_read(DICT_NEXT+DENT_TYPE) == 0, "dict[next] type fn should be 0 at start");
  fail_unless(dict_read(DICT_NEXT+DENT_PARAM) == 0, "dict[next] param should be 0 at start");
  fail_unless(dict_read(DICT_NEXT+DENT_PREV) == DICT_HEAD, "dict[next] prev should point back to head at start");
  dict_write(DICT_NEXT+DENT_NAME, 123);
  fail_unless(dict_read(DICT_NEXT+DENT_NAME) == 123, "dict[next] name should change when written");
  END
}

static void dict_move_to_next() {
  START
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
  END
}

static void dict_lookup() {
  START
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
  END
}

static void bytes_read_write() {
  START
  fail_unless(byte_read(POOL_NEXT) == 0, "pool next should be 0 at start");
  byte_write(POOL_NEXT, 123);
  fail_unless(byte_read(POOL_NEXT) == 123, "pool next should contain the updated value");

  word_write(POOL_NEXT, 0xaabbccdd);
  fail_unless(byte_read(POOL_NEXT+0) == 0xdd, "pool bytes should contain stored word (byte 0)");
  fail_unless(byte_read(POOL_NEXT+1) == 0xcc, "pool bytes should contain stored word (byte 1)");
  fail_unless(byte_read(POOL_NEXT+2) == 0xbb, "pool bytes should contain stored word (byte 2)");
  fail_unless(byte_read(POOL_NEXT+3) == 0xaa, "pool bytes should contain stored word (byte 3)");
  fail_unless(word_read(POOL_NEXT) == 0xaabbccdd, "word_read should read the stored value");
  END
}

static void byte_add() {
  START
  fail_unless(POOL_NEXT > POOL_HEAD, "pool next should be beyond pool head");

  address old_head = POOL_HEAD;
  address old_next = POOL_NEXT;
  type("a");
  address added = badd(INBUF_START);
  fail_unless(POOL_HEAD == old_next, "pool head should me moved to old next");
  fail_unless(added == POOL_HEAD, "badd should return new address");
  fail_unless(1 == word_read(POOL_HEAD+PENT_LEN), "supplied length should be stored with text");
  fail_unless(POOL_NEXT == POOL_HEAD + PENT_DATA + WORDSIZE, "pool next should be rounded up to the start of the next word block");
  END
}

static void byte_lookup_not_found() {
  START
  type("");
  fail_unless(blookup(INBUF_START) == 0, "bytes lookup of empty string should return 0");
  type("x");
  fail_unless(blookup(INBUF_START) == 0xFFFFFFFF, "bytes lookup of undefined string should return FFFFFFFF");
  END
}

static void byte_lookup_found_when_added() {
  START
  type("a");
  fail_unless(blookup(INBUF_START) == 0xFFFFFFFF, "bytes lookup of undefined string should return FFFFFFFF");

  address added = badd(INBUF_START);
  address found = blookup(INBUF_START);
  fail_unless(found == added, "bytes lookup of defined string should return its address");
  END
}

static void byte_lookup_found_by_length() {
  START
  type("x");
  fail_unless(blookup(INBUF_START) == 0xFFFFFFFF, "bytes lookup of undefined string should return FFFFFFFF");

  type("ab");
  address added1 = badd(INBUF_START);
  type("a");
  fail_unless(blookup(INBUF_START) == 0xFFFFFFFF, "bytes lookup of partial string should return FFFFFFFF");

  address added2 = badd(INBUF_START);
  fail_unless(blookup(INBUF_START) == added2, "bytes lookup of defined string should return its address");
  END
}

static void byte_lookup_found_by_content() {
  START
  type("x");
  fail_unless(blookup(INBUF_START) == 0xFFFFFFFF, "bytes lookup of undefined string should return FFFFFFFF");

  address added1 = badd(INBUF_START);

  type("y");
  fail_unless(blookup(INBUF_START) == 0xFFFFFFFF, "bytes lookup of partial string should return FFFFFFFF");

  address added2 = badd(INBUF_START);
  fail_unless(blookup(INBUF_START) == added2, "bytes lookup of defined string should return its address");
  END
}

static void not_a_number() {
  START
  bytes[INBUF_START] = 0;
  fail_unless(0 == number(INBUF_START), "empty string should not be a number");
  fail_unless(DS_TOP == DSTACK_START, "empty string should not push");

  bytes[INBUF_START] = 'x';
  bytes[INBUF_START+1] = 0;
  fail_unless(0 == number(INBUF_START), "'x' should not be a number");
  fail_unless(DS_TOP == DSTACK_START, "non-number string should not push");
  END
}

static void positive_number() {
  START
  type("1");
  fail_unless(1 == number(INBUF_START), "'1' should be a number");
  fail_unless(DS_TOP != DSTACK_START, "number string should push");
  fail_unless(1 == pop(), "number should be pushed");

  type("12");
  fail_unless(1 == number(INBUF_START), "'12' should be a number");
  fail_unless(DS_TOP != DSTACK_START, "number string should push");
  fail_unless(12 == pop(), "number should be pushed");

  type("123");
  fail_unless(1 == number(INBUF_START), "'123' should be a number");
  fail_unless(DS_TOP != DSTACK_START, "number string should push");
  fail_unless(123 == pop(), "number should be pushed");

  type("123p");
  fail_unless(0 == number(INBUF_START), "'123p' should not be a number");
  fail_unless(DS_TOP == DSTACK_START, "non-number string should not push");
  END
}

static void negative_number() {
  START
  type("-");
  fail_unless(0 == number(INBUF_START), "'-' should not be a number");
  fail_unless(DS_TOP == DSTACK_START, "non-number string should not push");

  type("-2");
  fail_unless(1 == number(INBUF_START), "'-2' should be a number");
  fail_unless(DS_TOP != DSTACK_START, "number string should push");
  fail_unless(-2 == pop(), "number should be pushed");

  type("-23");
  fail_unless(1 == number(INBUF_START), "'-23' should be a number");
  fail_unless(DS_TOP != DSTACK_START, "number string should push");
  fail_unless(-23 == pop(), "number should be pushed");
  END
}

static void eval_empty() {
  START
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at start");
  bytes[INBUF_START] = 0;
  evaluate(INBUF_START);
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
  word name = badd(INBUF_START);
  dict_write(DICT_NEXT+DENT_NAME, name);
  dict_write(DICT_NEXT+DENT_TYPE, (word)&dup);
  dict_write(DICT_NEXT+DENT_PARAM, 0);
  dadd();

  enter("96 hello");
  fail_unless(DS_TOP > DSTACK_START, "stack should not be empty at end");
  fail_unless(96 == pop(), "parameter value 96 should have been pushed once");
  fail_unless(96 == pop(), "parameter value 96 should have been pushed twice");
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at end, too");
  END
}

void define(const char* names, const char* bodys) {
  type(names);
  word name = badd(INBUF_START);
  type(bodys);
  word body = badd(INBUF_START);

  dict_write(DICT_NEXT+DENT_NAME, name);
  dict_write(DICT_NEXT+DENT_TYPE, (word)&evaluate);
  dict_write(DICT_NEXT+DENT_PARAM, body+PENT_DATA);
  dadd();
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

int reset() {
//  printf("*"); fflush(stdout);
  int i;

  word reset_dict[8] = {
      0, (word)&number, 0, 0,
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
  for (; i < POOL_BYTES + INBUF_BYTES; ++i) {
    bytes[i] = 0;
  }

  DS_TOP = DSTACK_START;
  RS_TOP = RSTACK_START;
  DICT_HEAD = DICT_START;
  DICT_NEXT = DICT_START+4;
  POOL_HEAD = POOL_START;
  POOL_NEXT = POOL_START+8;
  INBUF_IN = INBUF_START;
  INBUF_OUT = INBUF_START;
  INPUT_COUNT = 0;

  return 1;
}

#define test(fn) if (reset()) fn()

int main() {
  test(data_stack);
  test(return_stack);
  test(dict_read_write);
  test(dict_move_to_next);
  test(dict_lookup);
  test(bytes_read_write);
  test(byte_add);
  test(byte_lookup_not_found);
  test(byte_lookup_found_when_added);
  test(byte_lookup_found_by_length);
  test(byte_lookup_found_by_content);
  test(not_a_number);
  test(positive_number);
  test(negative_number);
  test(eval_empty);
  test(eval_number);
  test(eval_two_numbers);
  test(eval_word);
  test(eval_subroutine);

  if (fails) {
    printf("%d tests failed\n", fails);
  } else {
    puts("all tests passed");
  }
  return fails;
}
