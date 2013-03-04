#include <stdio.h>

#include "e13.h"
#include "debug.h"
#include "helper.h"

int tracing = 0;

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
#define complain(message) _fail_if(__FUNCTION__, __LINE__, 1, message);
#define fail_unless(expr, message) fail_if(!(expr), message)
#define fail(message) fail_if(1, message)

#define START //printf("%s:start\n", __FUNCTION__);
#define END //printf("%s:end\n", __FUNCTION__);

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

  dent_next();

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
  dent_next();
  fail_unless(dlup(123) == DICT_HEAD, "dict lookup should find item at head");

  address first_match = DICT_HEAD;
  word_write(DICT_NEXT+DENT_NAME, 456);
  dent_next();
  fail_unless(dlup(123) == first_match, "dict lookup should find item behind head");

  word_write(DICT_NEXT+DENT_NAME, 123);
  dent_next();
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

static void eval_two_numbers_with_trailing_space() {
  START
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at start");
  enter("23 97 ");
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
  word_write(DICT_NEXT+DENT_TYPE, (word)&primitive);
  word_write(DICT_NEXT+DENT_PARAM, (word)&prim_dup);
  dent_next();

  enter("96 hello");
  fail_unless(DS_TOP > DSTACK_START, "stack should not be empty at end");
  fail_unless(96 == pop(), "parameter value 96 should have been pushed once");
  fail_unless(96 == pop(), "parameter value 96 should have been pushed twice");
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at end, too");
  END
}

void define(const char* names, const char* bodys) {
  type(bodys);
  word body = pens(INBUF_START, INBUF_IN-INBUF_START);
  type(names);
  word name = pens(INBUF_START, INBUF_IN-INBUF_START);

  word_write(DICT_NEXT+DENT_NAME, name);
  word_write(DICT_NEXT+DENT_TYPE, (word)&definition);
  word_write(DICT_NEXT+DENT_PARAM, body);
  dent_next();
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

static void eval_prim_w_plus() {
  START
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at start");

  enter("96 W+");
  fail_unless(DS_TOP > DSTACK_START, "stack should not be empty at end");
  fail_unless(96 + WORDSIZE == pop(), "parameter value 96 should have been incremented by WORDSIZE");
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at end, too");
}

static void eval_def() {
  START
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty at start");

  enter("[123 W+] [ugh] def");
  fail_unless(DS_TOP == DSTACK_START, "stack should be empty after defining a new word");

  enter("ugh");
  fail_unless(DS_TOP > DSTACK_START, "stack should not be empty at end");
  fail_unless(123 + WORDSIZE == pop(), "parameter value from definition should have been pushed");
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
  test(eval_two_numbers_with_trailing_space);
  test(eval_string);
  test(eval_word);
  test(eval_subroutine);
  test(eval_prim_w_plus);
  test(eval_def);

  if (fails) {
    printf("%d tests failed\n", fails);
  } else {
    puts("all tests passed");
  }
  return fails;
}
