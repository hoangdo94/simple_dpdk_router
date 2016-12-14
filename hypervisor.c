#include "hypervisor.h"

static int get_vmmcall_number (const char *b)
{
	int r;
	asm volatile ("vmcall"
		      : "=a" (r)
		      : "a" (0), "b" (b)
		      : "memory");
	if (!r)
		return -1;
	return r;
}

static int vmmcall_reserve_memory;
static int vmmcall_isolate_core;

int request_reserve_memory(ulong start_addr, ulong size, mem_type type) {
	if (!vmmcall_reserve_memory) {
		vmmcall_reserve_memory = get_vmmcall_number("reserve_memory");
	}
	if (vmmcall_reserve_memory <= 0) {
		return -1;
	}

	int res;

	asm volatile ("vmcall"
		: "=a" (res)
		: "a" (vmmcall_reserve_memory), "b" (start_addr), "c" (size), "d" (type)
		: "memory");

	return res;
}

int request_isolate_core(proc_map_t *head) {
	if (!vmmcall_isolate_core) {
		vmmcall_isolate_core = get_vmmcall_number("isolate_core");
	}
	if (vmmcall_isolate_core <= 0) {
		return -1;
	}

	asm volatile("vmcall"
		:
		: "a" (vmmcall_isolate_core), "b" (head)
		: "memory");

	return 0;
}

static void proc_maps_print(proc_map_t *head) {
    proc_map_t *current = head;

    while (current != NULL) {
        printf("%08lx %08lx %d\n", current->start_addr, current->end_addr, current->need_remap);
        current = current->next;
    }
}

static void proc_maps_push(proc_map_t ** head, ulong start, ulong end, int map) {
	proc_map_t *new_node;
    new_node = malloc(sizeof(proc_map_t));

    new_node->start_addr = start;
    new_node->end_addr = end;
	new_node->need_remap = map;
    new_node->next = *head;
    *head = new_node;
}

proc_map_t *get_proc_maps(void) {
	FILE* proc_maps = fopen("/proc/self/maps", "r");

	char line[256];
	ulong start_address;
	ulong end_address;
	char perm[4];
	int is_binary;

	proc_map_t *head = NULL;

	while (fgets(line, 256, proc_maps) != NULL)
	{
		if(strstr(line, __progname) != NULL) {
			is_binary = 1;
		} else {
			is_binary = 0;
		}
		sscanf(line, "%08lx-%08lx %s\n", &start_address, &end_address, perm);
		printf("%08lx %08lx %s %d\n", start_address, end_address, perm, is_binary);
		proc_maps_push(&head, start_address, end_address, is_binary);
	}
	
	fclose(proc_maps);

	proc_maps_print(head);

	return head;
}
