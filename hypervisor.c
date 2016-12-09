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
static int vmmcall_test_call;

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

int test_call(void) {
	if (!vmmcall_test_call) {
		vmmcall_test_call = get_vmmcall_number("test_call");
	}
	if (vmmcall_test_call <= 0) {
		return -1;
	}

	asm volatile("vmcall"
		:
		: "a" (vmmcall_test_call)
		: "memory");

	return 0;
}


int print_proc_maps(void) {
	FILE* proc_maps = fopen("/proc/self/maps", "r");

	char line[256];
	ulong start_address;
	ulong end_address;
	char perm[4];
	int is_binary;

	while (fgets(line, 256, proc_maps) != NULL)
	{
		if(strstr(line, __progname) != NULL) {
			is_binary = 1;
		} else {
			is_binary = 0;
		}
		sscanf(line, "%08lx-%08lx %s\n", &start_address, &end_address, perm);
		printf("%08lx %08lx %s %d\n", start_address, end_address, perm, is_binary);
	}
	
	fclose(proc_maps);

	return 0;
}