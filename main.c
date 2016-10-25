#include "main.h"
#include "hypervisor.h"

int main(int argc, char **argv) {
	uint32_t lcore;
	int ret;

	/* Init EAL */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		return -1;
	argc -= ret;
	argv += ret;

	/* Parse application arguments (after the EAL ones) */
	ret = app_parse_args(argc, argv);
	if (ret < 0) {
		app_print_usage();
		return -1;
	}

	/* Init */
	app_init();

	/* Launch per-lcore init on every lcore */
	rte_eal_mp_remote_launch(app_lcore_main_loop, NULL, CALL_MASTER);
	RTE_LCORE_FOREACH_SLAVE(lcore) {
		if (rte_eal_wait_lcore(lcore) < 0)
			return -1;
	}
	
	return 0;
}

int app_lcore_main_loop(__attribute__((unused)) void *arg) {
	unsigned lcore;

	lcore = rte_lcore_id();

	if (lcore == app.core_rx) {
		#ifdef HYP
		test_call();
		#endif

		app_main_loop_rx();
    	return 0;
	}

	if (lcore == app.core_fw) {
		#ifdef HYP
		test_call();

		//isolate MMIO regions
		// request_reserve_memory(0xe1b00000, 0x20000, GUEST_PHYS);
		// request_reserve_memory(0xe1b20000, 0x20000, GUEST_PHYS);
		// request_reserve_memory(0xe1a40000, 0x40000, GUEST_PHYS);
		// request_reserve_memory(0xe1ac0000, 0x40000, GUEST_PHYS);
		#endif

	 	app_main_loop_fw();
	 	return 0;
	}

	if (lcore == app.core_tx) {
		#ifdef	HYP
		test_call();

		//isolate TX descriptors
		int eth_dev_count = rte_eth_dev_count();
		int i;
		for (i=0; i<eth_dev_count; i++) {
			struct rte_eth_dev *dev = &rte_eth_devices[i];
			int j;
			for (j=0; j<dev->data->nb_tx_queues; j++) {
				void *txq = dev->data->tx_queues[j];
				//Magic numbers (for e1000 driver)
				uint64_t *addr = (uint64_t *) ((uint64_t) txq + 4);
				uint64_t *nb = (uint64_t *) ((uint64_t) txq + 20);
				uint64_t struct_size = 16;

				ulong start_addr = (ulong) *addr;
				ulong size = (ulong) (*nb) * struct_size;
				printf("request");
				printf("\tstart_addr %lx", start_addr);
				printf("\tsize %lx\n", size);
				request_reserve_memory(start_addr, size, GUEST_PHYS);
			}
		}
		#endif

		app_main_loop_tx();
		return 0;
	}

	return 0;
}
