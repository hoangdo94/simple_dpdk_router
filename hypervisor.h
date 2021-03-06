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

typedef struct proc_map {
	ulong start_addr;
	ulong end_addr;
	int need_remap;
	struct proc_map *next;
} proc_map_t;

int request_reserve_memory(ulong start_addr, ulong size, mem_type type);
int request_isolate_core(proc_map_t *head);
proc_map_t *get_proc_maps(void);
