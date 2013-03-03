#include <stdio.h>

#include "e13.h"

#define N_DEVICES 32

enum device_operation { READBYTE, WRITEBYTE, READWORD, WRITEWORD };
typedef word (*devicefn)(enum device_operation op, address p, word v);

struct device {
  int active;
  const char* name;
  address start;
  address end;
  devicefn fn;
};

// the memory model, simulating basic RAM so that I can get as close to Chuck's original design as possible.
byte bytes[MEMORY_SIZE];
byte* memory_start = bytes;

// memory-mapped devices, allowing dynamic plug/unplug
struct device devices[N_DEVICES] = { 0 };

void map_device(const char* name, address start, address end, devicefn fn) {
  for (int i = 0; i < N_DEVICES; ++i) {
    struct device* device = &devices[i];
    if (!device->active) {
      device->name = name;
      device->start = start;
      device->end = end;
      device->fn = fn;
      device->active = 1;
      return;
    }
  }
}

devicefn find_device(address p) {
  for (int i = 0; i < N_DEVICES; ++i) {
    struct device* device = &devices[i];
    if (device->active && p >= device->start && p < device->end) {
        return device->fn;
    }
  }

  return 0;
}

void dump_devices(void) {
  printf("Devices:\n");
  for (int i = 0; i < N_DEVICES; ++i) {
    struct device* device = &devices[i];
    if (device->active) {
      printf(" %s: %08x-%08x\n", device->name, device->start, device->end);
    }
  }
}

// memory access functions, with extra protection for tests
uint8_t byte_read(address p) {
  if (p < (word)bytes || p >= (word)(bytes + MEMORY_SIZE)) {
    devicefn fn = find_device(p);
    if (0 != fn) {
      return (byte)fn(READBYTE, p, 0);
    } else {
      printf("attempt to read byte outside memory map[%08x-%08x] (p=%08x)\n", bytes, bytes + MEMORY_SIZE, p);
      dump_devices();
      return 0;
    }
  }
  return *(byte*)p;
}

void byte_write(address p, byte v) {
  if (p < (word)bytes || p >= (word)(bytes + MEMORY_SIZE)) {
    devicefn fn = find_device(p);
    if (0 != fn) {
      fn(WRITEBYTE, p, (word)v);
      return;
    } else {
      printf("attempt to write byte outside memory map[%08x-%08x] (p=%08x) (v=%08x)\n", bytes, bytes + MEMORY_SIZE, p, v);
      dump_devices();
      return;
    }
  }
  *(byte*)p = v;
}

word word_read(address p) {
  if (p < (word)bytes || p >= (word)(bytes + MEMORY_SIZE)) {
    devicefn fn = find_device(p);
    if (0 != fn) {
      return fn(READWORD, p, 0);
    } else {
      printf("attempt to read word outside memory map[%08x-%08x] (p=%08x)\n", bytes, bytes + MEMORY_SIZE, p);
      dump_devices();
      return 0;
    }
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

void word_write(address p, word v) {
  if (p < (word)bytes || p >= (word)(bytes + MEMORY_SIZE)) {
    devicefn fn = find_device(p);
    if (0 != fn) {
      fn(WRITEWORD, p, v);
      return;
    } else {
      printf("attempt to write word outside memory map[%08x-%08x] (p=%08x) (v=%08x)\n", bytes, bytes + MEMORY_SIZE, p, v);
      dump_devices();
      return;
    }
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

#define AUX_MU_IO_REG 0x20215004
word uart_fn(enum device_operation op, address p, word v) {
  if (WRITEBYTE == op && AUX_MU_IO_REG == p) {
    char c = (char)v;
    putchar(c);
  } else {
    printf("unsupported device access - op=%d, p=%08x, v=%08x", op, p, v);
  }
  return 0;
}

void hardware_init(void) {
  for (int i = 0; i < N_DEVICES; ++i) devices[i].active = 0;
  map_device("UART", 0x20215000, 0x2021506C, &uart_fn);
}

