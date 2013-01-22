#include <stdio.h>

#include "io.h"

void console_write_char(char c) {
  putchar(c);
}

void console_write_string(const char* s) {
  while (*s) {
    putchar(*s++);
  }
}

void console_write_number(int n) {
  printf("%d", n);
}
