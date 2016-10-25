#include "hypervisor.h"

static int get_vmmcall_number (void* b)
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
