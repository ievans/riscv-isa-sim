#pragma once

/* obtained from arch/riscv/include/asm/page.h */
#define PAGE_OFFSET		0xffffffff80000000UL
/* obtained from arch/riscv/include/asm/processor.h */
#define TASK_SIZE		(1UL << 31)

/* 	I don't know if these are correct, but it doesn't matter,
	as these are only used in "interesting" random number
	generation anyway. */
#define KERNEL_ADDR		0x0
#define MODULE_ADDR		KERNEL_ADDR + 0x80000000UL

/* obtained from arch/riscv/include/asm/page.h */
#define PAGE_SHIFT		13
/* obtained from arch/riscv/include/asm/pgtable.h */
#define PTE_FILE_MAX_BITS	64 - PAGE_SHIFT

/* This seems like a NULL definition. */
#define PTRACE_GETREGS		0
#define PTRACE_GETFPREGS	0
#define PTRACE_SETREGS		0
#define PTRACE_SETFPREGS	0

#define SYSCALLS syscalls_riscv
