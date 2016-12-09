#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *__progname;

enum mem_type_t {
	GUEST_PHYS = 0x1,
	GUEST_VIRT = 0x2
};

typedef unsigned long ulong;
typedef enum mem_type_t mem_type;

struct proc_map {
	struct proc_map *next;
	ulong start_addr;
	ulong end_addr;
	int need_remap;
};

int request_reserve_memory(ulong start_addr, ulong size, mem_type type);
int test_call(void);
int print_proc_maps(void);