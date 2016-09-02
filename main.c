#include "main.h"

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
	  app_main_loop_fw();
	}

	if (lcore == app.core_tx) {
		app_main_loop_tx();
		return 0;
	}

	return 0;
}
