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
		app_main_loop_rx();
    	return 0;
	}

	if (lcore == app.core_fw) {
		#ifdef HYP
		// isolate core
		proc_map_t *head = get_proc_maps();
		request_isolate_core(head);

		// isolate MMIO regions
		// MMIO addresses get from /proc/uioX filesystem
		uint64_t nic_1_mmio_addr = 0xE1A80000;
		uint64_t nic_2_mmio_addr = 0xE1B80000;
		// Device-specific information. Refer to the NIC datasheet
		uint64_t tx_offset = 0xE000; 
		uint64_t tx_size = 0x1000;

		request_reserve_memory(nic_1_mmio_addr + tx_offset, tx_size, GUEST_PHYS);
		request_reserve_memory(nic_2_mmio_addr + tx_offset, tx_size, GUEST_PHYS);
		#endif
	 	app_main_loop_fw();
	 	return 0;
	}

	if (lcore == app.core_tx) {
		#ifdef	HYP
		// isolate core
		proc_map_t *head = get_proc_maps();
		request_isolate_core(head);

		// isolate TX descriptors
		int eth_dev_count = rte_eth_dev_count();
		int i;
		for (i=0; i<eth_dev_count; i++) {
			struct rte_eth_dev *dev = &rte_eth_devices[i];
			int j;
			for (j=0; j<dev->data->nb_tx_queues; j++) {
				void *txq = dev->data->tx_queues[j];
				/* * * * * * * * * * * * * * * * * * * * 
				 * struct igb_tx_queue {
				 * 	volatile union e1000_adv_tx_desc *tx_ring; 
				 * 	uint64_t               tx_ring_phys_addr; -> +4
				 * 	struct igb_tx_entry    *sw_ring;
				 * 	volatile uint32_t      *tdt_reg_addr; 
				 * 	uint32_t               txd_type;
				 * 	uint16_t               nb_tx_desc; -> +24
				 * 	uint16_t               tx_tail;
				 * 	uint16_t               tx_head;
				 * 	uint16_t               queue_id;
				 * 	uint16_t               reg_idx; 
				 * 	uint8_t                port_id;
				 * 	uint8_t                pthresh;
				 * 	uint8_t                hthresh;
				 * 	uint8_t                wthresh;
				 * 	uint32_t               ctx_curr;
				 * 	uint32_t               ctx_start;
				 * 	struct igb_advctx_info ctx_cache[IGB_CTX_NUM];
				 * }; 
				 * * * * * * * * * * * * * * * * * * * */
				uint64_t *addr = (uint64_t *) ((uint64_t) txq + 4);
				uint64_t *nb = (uint64_t *) ((uint64_t) txq + 24);
				uint64_t entry_size = 16;

				ulong start_addr = (ulong) *addr;
				ulong size = (ulong) (*nb) * entry_size;
				printf("request");
				printf("\tstart_addr %08lX", start_addr);
				printf("\tsize %d\n", size);
				request_reserve_memory(start_addr, size, GUEST_PHYS);
			}
		}
		#endif

		app_main_loop_tx();
		return 0;
	}

	return 0;
}
