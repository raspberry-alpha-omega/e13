wordsize = 4

.globl _start
.globl memory_start
.extern main

.text
_start:
  call main
  movl $1, %eax
  xor %ebx, %ebx
  int $0x80

.data
.align wordsize
memory_start:
