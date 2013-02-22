wordsize = 4

.globl _start
.globl memory_start

_start:
  call main
  movl $1, %eax
  xor %ebx, %ebx
  int $0x80

.align wordsize
memory_start:
