#pragma once

#define KERNEL_ADDR 0xc0100220
#define MODULE_ADDR 0xa0000000	// FIXME: Placeholder
#define PAGE_OFFSET 0x80000000
#define TASK_SIZE (PAGE_OFFSET)
#define PAGE_SHIFT 12
#define PTE_FILE_MAX_BITS 31
#define SYSCALL_OFFSET 4000

#define SYSCALLS syscalls_mips
