wordsize = 8

.globl _start
.globl memory_start
.globl sys_consts
.globl sys_vars
.globl soft_start

_start:
  call main
  movl $1, %eax
  xor %ebx, %ebx
  int $0x80

.align wordsize
memory_start:
sys_consts = memory_start
sys_vars = sys_consts + wordsize * 12
