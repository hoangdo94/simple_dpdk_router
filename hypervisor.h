enum mem_type_t {
	GUEST_PHYS = 0x1,
	GUEST_VIRT = 0x2
};

typedef unsigned long ulong;
typedef enum mem_type_t mem_type;

int request_reserve_memory(ulong start_addr, ulong size, mem_type type);
int test_call(void);