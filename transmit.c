#include "main.h"

void app_main_loop_tx(void) {
	uint32_t i;
	// uint64_t c1, c2;

	RTE_LOG(INFO, USER1, "Core %u is doing TX\n", rte_lcore_id());

	//here ??

	for (i = 0; ; i = ((i + 1) & (app.n_ports - 1))) {
		uint16_t n_mbufs, n_pkts;
		int ret;

		n_mbufs = app.mbuf_tx[i].n_mbufs;

		ret = rte_ring_sc_dequeue_bulk(
			app.rings_tx[i],
			(void **) &app.mbuf_tx[i].array[n_mbufs],
			app.burst_size_tx_read);

		if (ret == -ENOENT)
			continue;

		n_mbufs += app.burst_size_tx_read;

		if (n_mbufs < app.burst_size_tx_write) {
			app.mbuf_tx[i].n_mbufs = n_mbufs;
			continue;
		}

		// c1 = rte_rdtsc();
		n_pkts = rte_eth_tx_burst(
			app.ports[i],
			0,
			app.mbuf_tx[i].array,
			n_mbufs);
		// c2 = rte_rdtsc();

		// printf("TX: %d cycles/packet\n", (c2-c1)/n_pkts);
		
		if (n_pkts < n_mbufs) {
			uint16_t k;

			for (k = n_pkts; k < n_mbufs; k++) {
				struct rte_mbuf *pkt_to_free;

				pkt_to_free = app.mbuf_tx[i].array[k];
				rte_pktmbuf_free(pkt_to_free);
			}
		}

		app.mbuf_tx[i].n_mbufs = 0;
	}
}
