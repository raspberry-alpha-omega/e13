wordsize = 8
.globl _start
.align 8
_start:
  call main
  movl $1, %eax
  xorl %ebx,%ebx
  int $0x80

.globl memory_start
.globl sys_consts
.globl sys_vars
.globl soft_start
memory_start:
sys_consts:
.space wordsize * 12, 0
sys_vars:
.space wordsize * 8, 0
soft_start:
