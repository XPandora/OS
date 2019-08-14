/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>

#include <kern/picirq.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.section .user_mapped.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
TRAPHANDLER_NOEC( T_DIVIDE_ENTRY  , T_DIVIDE ) 
TRAPHANDLER_NOEC( T_DEBUG_ENTRY   , T_DEBUG  )  
TRAPHANDLER_NOEC( T_NMI_ENTRY     , T_NMI    ) 
TRAPHANDLER_NOEC( T_BRKPT_ENTRY   , T_BRKPT  )  
TRAPHANDLER_NOEC( T_OFLOW_ENTRY   , T_OFLOW  )  
TRAPHANDLER_NOEC( T_BOUND_ENTRY   , T_BOUND  )  
TRAPHANDLER_NOEC( T_ILLOP_ENTRY   , T_ILLOP  )  
TRAPHANDLER_NOEC( T_DEVICE_ENTRY  , T_DEVICE )  
TRAPHANDLER     ( T_DBLFLT_ENTRY  , T_DBLFLT )  

TRAPHANDLER     ( T_TSS_ENTRY     , T_TSS    ) 
TRAPHANDLER     ( T_SEGNP_ENTRY   , T_SEGNP  )  
TRAPHANDLER     ( T_STACK_ENTRY   , T_STACK  )  
TRAPHANDLER     ( T_GPFLT_ENTRY   , T_GPFLT  )  
TRAPHANDLER     ( T_PGFLT_ENTRY   , T_PGFLT  )  
 
TRAPHANDLER_NOEC( T_FPERR_ENTRY   , T_FPERR  )  
TRAPHANDLER_NOEC( T_ALIGN_ENTRY   , T_ALIGN  ) 
TRAPHANDLER_NOEC( T_MCHK_ENTRY    , T_MCHK   ) 
TRAPHANDLER_NOEC( T_SIMDERR_ENTRY , T_SIMDERR)
TRAPHANDLER_NOEC( T_SYSCALL_ENTRY , T_SYSCALL)  

TRAPHANDLER_NOEC(IRQ0_ENTRY, IRQ_OFFSET)
TRAPHANDLER_NOEC(IRQ1_ENTRY, IRQ_OFFSET + 1)
TRAPHANDLER_NOEC(IRQ2_ENTRY, IRQ_OFFSET + 2)
TRAPHANDLER_NOEC(IRQ3_ENTRY, IRQ_OFFSET + 3)
TRAPHANDLER_NOEC(IRQ4_ENTRY, IRQ_OFFSET + 4)
TRAPHANDLER_NOEC(IRQ5_ENTRY, IRQ_OFFSET + 5)
TRAPHANDLER_NOEC(IRQ6_ENTRY, IRQ_OFFSET + 6)
TRAPHANDLER_NOEC(IRQ7_ENTRY, IRQ_OFFSET + 7)
TRAPHANDLER_NOEC(IRQ8_ENTRY, IRQ_OFFSET + 8)
TRAPHANDLER_NOEC(IRQ9_ENTRY, IRQ_OFFSET + 9)
TRAPHANDLER_NOEC(IRQ10_ENTRY, IRQ_OFFSET + 10)
TRAPHANDLER_NOEC(IRQ11_ENTRY, IRQ_OFFSET + 11)
TRAPHANDLER_NOEC(IRQ12_ENTRY, IRQ_OFFSET + 12)
TRAPHANDLER_NOEC(IRQ13_ENTRY, IRQ_OFFSET + 13)
TRAPHANDLER_NOEC(IRQ14_ENTRY, IRQ_OFFSET + 14)
TRAPHANDLER_NOEC(IRQ15_ENTRY, IRQ_OFFSET + 15)

TRAPHANDLER_NOEC(IRQ_ERROR_ENTRY, IRQ_OFFSET + IRQ_ERROR)
/*
 * Lab 3: Your code here for _alltraps
 */
 
.globl _alltraps;
.type _alltraps, @function;
.align 2;
_alltraps:
	pushl %ds
	pushl %es
	pushal
	movl $GD_KD, %eax
	movw %ax, %ds
	movw %ax, %es
	pushl %esp
	call switch_and_trap

.globl sysenter_handler;
.type sysenter_handler, @function;
.align 2;
sysenter_handler:
	pushw $0
  pushw $GD_UD | 3
  pushl %ebp
  pushfl
  pushw $0
  pushw $GD_UT | 3
  pushl %esi
  pushl $0
  pushl $0
  pushw $0
  pushw %ds
  pushw $0
  pushw %es
  pushal
  pushl %esp
	pushl 0x4(%ebp)
  pushl %edi
  pushl %ebx
  pushl %ecx
  pushl %edx
  pushl %eax
  call _syscall
  movl %ebp, %ecx
  movl %esi, %edx
  sti
  sysexit

