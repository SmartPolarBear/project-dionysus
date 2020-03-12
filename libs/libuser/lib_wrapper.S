.code64

#include "sys/mmu_asm.h"
#include "sys/memlayout_asm.h"

.data
// Define the global symbols 'envs', 'pages', 'uvpt', and 'uvpd'
// so that they can be used in C as if they were ordinary global arrays.
//	.globl envs
//	.set envs, UENVS
//	.globl pages
//	.set pages, UPAGES
//	.globl uvpt
//	.set uvpt, UVPT
//	.globl uvpd
//	.set uvpd, (UVPT+(UVPT>>12)*4)

// Entrypoint - this is where the kernel (or our parent environment)
// starts us running when we are initially loaded into a new environment.
.text
.globl _start
_start:
	int $128
	call libmain
spin:
 jmp spin