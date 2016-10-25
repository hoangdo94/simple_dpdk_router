#include "main.h"

void app_main_loop_rx(void) {
	uint32_t i;
	int ret;
	// uint64_t c1, c2;

	RTE_LOG(INFO, USER1, "Core %u is doing RX\n", rte_lcore_id());

	for (i = 0; ; i = ((i + 1) & (app.n_ports - 1))) {
		uint16_t n_mbufs;

		// c1 = rte_rdtsc();
		n_mbufs = rte_eth_rx_burst(
			app.ports[i],
			0,
			app.mbuf_rx.array,
			app.burst_size_rx_read);
		// c2 = rte_rdtsc();

		if (n_mbufs == 0)
			continue;

		// printf("RX: %d cycles/packet\n", (c2-c1)/n_mbufs);
		
		do {
			ret = rte_ring_sp_enqueue_bulk(
				app.rings_rx[i],
				(void **) app.mbuf_rx.array,
				n_mbufs);
		} while (ret < 0);
	}
}
