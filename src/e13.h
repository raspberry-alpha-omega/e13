#ifndef E13_H
#define E13_H

#define GETBYTE(i) (mem[i] & 0xFF)
enum label {
  HEAD,
  INCOUNT,
  INBUFFER,
  DICT = INBUFFER + 30,
  STACK = DICT + 32,
  END = STACK + 32
};

extern int mem[];

#endif
