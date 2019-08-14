// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE 80 // enough for one VGA text line

struct Command
{
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char **argv, struct Trapframe *tf);
};

static struct Command commands[] = {
		{"help", "Display this list of commands", mon_help},
		{"kerninfo", "Display information about the kernel", mon_kerninfo},
		{"backtrace", "Display stack backtrace", mon_backtrace},
		{"time", "Display time a command costs", mon_time},
		{"showmappings", "Display physical memory mappings", mon_showmappings},
		{"setpermission", "Set permission of mapping", mon_setpermission},
		{"dumpva", "Dump memory content by virtual address", mon_dumpva},
		{"dumppa", "Dump memory content by physical address", mon_dumppa}};

/***** Implementations of basic kernel monitor commands *****/

int mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
					ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
	if (argc != 3)
	{
		cprintf("Usage: showmappins [start address] [end address]\n");
		return -1;
	}

	char *errChar;
	uintptr_t start_addr = strtol(argv[1], &errChar, 16);

	if (*errChar)
	{
		cprintf("Invalid virtual address: %s.\n", argv[1]);
		return -1;
	}
	uintptr_t end_addr = strtol(argv[2], &errChar, 16);

	if (*errChar)
	{
		cprintf("Invalid virtual address: %s.\n", argv[2]);
		return -1;
	}
	if (start_addr > end_addr)
	{
		cprintf("Start address must be lower than end address\n");
		return -1;
	}

	while (start_addr < end_addr)
	{
		pte_t *pte = pgdir_walk(kern_pgdir, (void *)start_addr, 0);

		if (pte == NULL)
		{
			cprintf("virtual addr %08x: not mapped\n", start_addr);
			start_addr += PGSIZE;
			continue;
		}
		else
		{
			cprintf("virtual addr %08x: physical addr %08x, PTE_P: %x, PTE_W: %x, PTE_U: %x\n",
							start_addr, PTE_ADDR(*pte), (*pte & PTE_P) > 0, (*pte & PTE_W) > 0, (*pte & PTE_U) > 0);
		}
		if (*pte & PTE_PS)
		{
			start_addr += PTSIZE;
		}
		else
		{
			start_addr += PGSIZE;
		}
	}

	return 0;
}

int mon_setpermission(int argc, char **argv, struct Trapframe *tf)
{
	if (argc != 3)
	{
		cprintf("Usage: setpermission [virtual address] [u/r/w]\n");
		return -1;
	}
	pte_t *pte = pgdir_walk(kern_pgdir, (void *)argv[1], 0);

	if (pte == NULL || !(*pte & PTE_P))
	{
		cprintf("Virtual address %08x: not mapped\n");
		return -1;
	}
	char permission = argv[2][0];
	switch (permission)
	{
	case 'u':
		*pte = *pte | PTE_U;
		break;
	case 'r':
		*pte = *pte & (~PTE_W);
		break;
	case 'w':
		*pte = *pte | PTE_W;
		break;
	default:
		cprintf("Usage: setpermission [virtualaddress] [u/r/w]\n");
		return -1;
	}

	cprintf("After setting, PTE_U: %x, PTE_W: %x\n", (*pte & PTE_U) > 0, (*pte & PTE_W) > 0);
	return 0;
}

int mon_dumpva(int argc, char **argv, struct Trapframe *tf)
{
	if (argc != 3)
	{
		cprintf("Usage: dumpva [start va] [end va]\n");
		return -1;
	}

	char *errChar;
	uintptr_t start_va = strtol(argv[1], &errChar, 16);

	if (*errChar)
	{
		cprintf("Invalid virtual address: %s.\n", argv[1]);
		return -1;
	}
	uintptr_t end_va = strtol(argv[2], &errChar, 16);

	if (*errChar)
	{
		cprintf("Invalid virtual address: %s.\n", argv[2]);
		return -1;
	}
	if (start_va > end_va)
	{
		cprintf("Start address must be lower than end address\n");
		return -1;
	}

	uint32_t length = end_va - start_va;
	for (int i = 0; i < length; i++)
	{
		cprintf("%02x ", *((unsigned char *)start_va + i));
		if ((i + 1) % 8 == 0)
		{
			cprintf("\n");
		}
	}
	if (length % 8 != 0)
	{
		cprintf("\n");
	}

	return 0;
}

int mon_dumppa(int argc, char **argv, struct Trapframe *tf)
{
	if (argc != 3)
	{
		cprintf("Usage: dumpva [start va] [end va]\n");
		return -1;
	}

	char *errChar;
	uintptr_t start_pa = strtol(argv[1], &errChar, 16);

	if (*errChar)
	{
		cprintf("Invalid virtual address: %s.\n", argv[1]);
		return -1;
	}
	uintptr_t end_pa = strtol(argv[2], &errChar, 16);

	if (*errChar)
	{
		cprintf("Invalid virtual address: %s.\n", argv[2]);
		return -1;
	}
	if (start_pa > end_pa)
	{
		cprintf("Start address must be lower than end address\n");
		return -1;
	}

	uint32_t length = end_pa - start_pa;
	for (int i = 0; i < length; i++)
	{
		cprintf("%02x ", *((unsigned char *)KADDR(start_pa) + i));
		if ((i + 1) % 8 == 0)
		{
			cprintf("\n");
		}
	}
	if (length % 8 != 0)
	{
		cprintf("\n");
	}

	return 0;
}

// Lab1 only
// read the pointer to the retaddr on the stack
static uint32_t
read_pretaddr()
{
	uint32_t pretaddr;
	__asm __volatile("leal 4(%%ebp), %0"
									 : "=r"(pretaddr));
	return pretaddr;
}

void do_overflow(void)
{
	cprintf("Overflow success\n");
}

void start_overflow(void)
{
	// You should use a techique similar to buffer overflow
	// to invoke the do_overflow function and
	// the procedure must return normally.

	// And you must use the "cprintf" function with %n specifier
	// you augmented in the "Exercise 9" to do this job.

	// hint: You can use the read_pretaddr function to retrieve
	//       the pointer to the function call return address;

	char str[256] = {};
	int nstr = 0;
	// Your code here.

	char *pret_addr = (char *)read_pretaddr();
	memset(str, 0, 256);
	uint32_t overflow_addr = (uint32_t)do_overflow;
	cprintf("%x \n", overflow_addr);

	for (int i = 0; i < 3; ++i)
	{
		cprintf("%*d%n\n", pret_addr[i] & 0xFF, 1, pret_addr + 4 + i);
	}
	for (int i = 0; i < 3; ++i)
	{
		cprintf("%*d%n\n", (overflow_addr >> (8 * i)) & 0xFF, 1, pret_addr + i);
	}

	cprintf("address%x \n", *(uint32_t *)pret_addr);
}

void overflow_me(void)
{
	// cprintf("maintain this function!\n");
	start_overflow();
}

int mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	// do_overflow();
	overflow_me();
	cprintf("Stack backtrace:\n");

	uint32_t *ebp = (uint32_t *)read_ebp();

	while (ebp)
	{
		struct Eipdebuginfo info;

		while (ebp)
		{
			cprintf("  eip %08x  ebp %08x  args %08x %08x %08x %08x %08x\n", ebp[1], ebp, ebp[2], ebp[3], ebp[4], ebp[5], ebp[6]);
			debuginfo_eip(ebp[1], &info);
			cprintf("         %s:%u %.*s+%u\n", info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name, ebp[1] - (uint32_t)info.eip_fn_addr);

			ebp = (uint32_t *)ebp[0];
		}
	}

	cprintf("Backtrace success\n");
	return 0;
}

/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1)
	{
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS - 1)
		{
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++)
	{
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}

int mon_time(int argc, char **argv, struct Trapframe *tf)
{
	if (argc < 2)
	{
		cprintf("Usage: time cmd\n");
		return 0;
	}

	uint64_t start = read_tsc();
	int ret = runcmd(argv[1], tf);
	uint64_t end = read_tsc();

	cprintf("%s cycles: %llu\n", argv[1], end - start);

	return 0;
}


