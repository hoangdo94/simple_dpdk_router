#include "main.h"

void app_main_loop_tx(void) {
	uint32_t i;

	RTE_LOG(INFO, USER1, "Core %u is doing TX\n", rte_lcore_id());

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

		n_pkts = rte_eth_tx_burst(
			app.ports[i],
			0,
			app.mbuf_tx[i].array,
			n_mbufs);

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
