#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>

#include <rte_common.h>
#include <rte_eal.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_log.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_byteorder.h>
#include <rte_port_ring.h>
#include <rte_table_acl.h>
#include <rte_pipeline.h>
#include <rte_cycles.h>

#ifndef APP_MBUF_ARRAY_SIZE
#define APP_MBUF_ARRAY_SIZE 256
#endif

struct app_mbuf_array {
	struct rte_mbuf *array[APP_MBUF_ARRAY_SIZE];
	uint16_t n_mbufs;
};

#ifndef APP_MAX_PORTS
#define APP_MAX_PORTS 4
#endif

struct app_params {
	/* CPU cores */
	uint32_t core_rx;
	uint32_t core_fw;
	uint32_t core_tx;

	/* Ports*/
	uint32_t ports[APP_MAX_PORTS];
	uint32_t n_ports;
	uint32_t port_rx_ring_size;
	uint32_t port_tx_ring_size;

	/* Rings */
	struct rte_ring *rings_rx[APP_MAX_PORTS];
	struct rte_ring *rings_tx[APP_MAX_PORTS];
	uint32_t ring_rx_size;
	uint32_t ring_tx_size;

	/* Internal buffers */
	struct app_mbuf_array mbuf_rx;
	struct app_mbuf_array mbuf_tx[APP_MAX_PORTS];

	/* Buffer pool */
	struct rte_mempool *pool;
	uint32_t pool_buffer_size;
	uint32_t pool_size;
	uint32_t pool_cache_size;

	/* Burst sizes */
	uint32_t burst_size_rx_read;
	uint32_t burst_size_rx_write;
	uint32_t burst_size_fw_read;
	uint32_t burst_size_fw_write;
	uint32_t burst_size_tx_read;
	uint32_t burst_size_tx_write;

	/* rule path */
	char *rule_path;

} __rte_cache_aligned;

extern struct app_params app;

int app_parse_args(int argc, char **argv);
void app_print_usage(void);
void app_init(void);
int app_lcore_main_loop(void *arg);
void app_main_loop_rx(void);
void app_main_loop_fw(void);
void app_main_loop_tx(void);

#endif /* _MAIN_H_ */
